#include "trc/ShaderLoader.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <shader_tools/ShaderDocument.h>
#include <spirv/FileIncluder.h>
#include <trc_util/Util.h>
#include <trc_util/Timer.h>

#include "trc/Types.h"
#include "trc/base/Logging.h"
#include "trc/base/ShaderProgram.h"



namespace trc
{

namespace nl = nlohmann;

ShaderLoader::ShaderLoader(
    std::vector<fs::path> _includePaths,
    fs::path binaryPath,
    std::optional<fs::path> shaderDbFile,
    shaderc::CompileOptions opts)
    :
    includePaths(std::move(_includePaths)),
    outDir(std::move(binaryPath)),
    compileOpts(std::move(opts))
{
    if (shaderDbFile.has_value() && fs::is_regular_file(*shaderDbFile))
    {
        std::ifstream file(*shaderDbFile);
        if (!file.is_open()) {
            throw std::invalid_argument("[In ShaderLoader::ShaderLoader]: Unable to open shader"
                                        " database " + shaderDbFile->string());
        }

        try {
            shaderDatabase = ShaderDB(nl::json::parse(file));
        }
        catch (const nl::json::parse_error& err) {
            throw std::invalid_argument("[In ShaderLoader::ShaderLoader]: Unable to load shader"
                                        " database: " + std::string(err.what()));
        }
    }

    if (fs::exists(outDir) && !fs::is_directory(outDir)) {
        throw std::invalid_argument("[In ShaderLoader::ShaderLoader]: Object at binary directory"
                                    " path " + outDir.string() + " exists but is not a directory!");
    }
    fs::create_directories(outDir);

    compileOpts.SetIncluder(std::make_unique<spirv::FileIncluder>(includePaths));
}

auto ShaderLoader::makeDefaultOptions() -> shaderc::CompileOptions
{
    shaderc::CompileOptions opts;

#ifdef TRC_FLIP_Y_PROJECTION
    opts.AddMacroDefinition("TRC_FLIP_Y_AXIS");
#endif
    opts.SetTargetSpirv(shaderc_spirv_version_1_5);
    opts.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan,
                              shaderc_env_version_vulkan_1_3);
    opts.SetOptimizationLevel(shaderc_optimization_level_performance);

    return opts;
}

auto ShaderLoader::load(const ShaderPath& shaderPath) const -> std::vector<ui32>
{
    /**
     * The longest possible dependency chain is:
     *
     *     file  -->  GLSL source  -->  SPIRV code
     *
     * where `file` contains unset variables. The intermediate 'true' GLSL
     * source is generated in `findFile` if it is outdated.
     */

    if (const auto srcPath = findShaderSource(shaderPath.getSourceName()))
    {
        assert(fs::is_regular_file(*srcPath));

        const auto binPath = outDir / shaderPath.getBinaryName();
        if (binaryDirty(*srcPath, binPath)) {
            return compile(*srcPath, binPath);
        }
        return readSpirvFile(binPath);
    }

    throw std::out_of_range("[In ShaderLoader::load]: Shader source "
                            + shaderPath.getSourceName().string() + " not found.");
}

auto ShaderLoader::findDeps(const fs::path& path) const -> std::vector<fs::path>
{
    // Try to look up dependencies in the cache first
    {
        std::scoped_lock _lock{ sourceDepsCacheLock };
        auto [it, notCached] = sourceDepsCache.try_emplace(path);
        if (!notCached) {
            return it->second;
        }
    }

    // Not cached - recursively gather #include directives from the file.
    std::vector<fs::path> res;

    std::ifstream file{ path };
    std::string line;
    while (std::getline(file, line))
    {
        if (auto begin = line.find("#include"); begin != std::string::npos)
        {
            begin = line.find('"', begin);
            if (begin == std::string::npos) {
                continue;
            }
            ++begin;
            auto end = line.find('"', begin);
            if (end == std::string::npos)
            {
                log::debug << "[ShaderLoader] Weird occurrence: Syntactically incorrect #include"
                           << " statement in " << path << " line \"" << line << "\".";
                continue;
            }

            const util::Pathlet depPath{ line.substr(begin, end - begin) };
            if (auto dep = findShaderSource(depPath))
            {
                res.emplace_back(*dep);
                auto recursive = findDeps(*dep);
                std::move(recursive.begin(), recursive.end(), std::back_inserter(res));
            }
            else {
                log::debug << "[ShaderLoader] Include \"" << depPath << "\" not found"
                           << " (included from " << path << ").";
            }
        }
    }

    // Store results in the cache.
    {
        std::scoped_lock _lock{ sourceDepsCacheLock };
        assert(sourceDepsCache.contains(path));
        sourceDepsCache.at(path) = res;
    }

    return res;
}

bool ShaderLoader::binaryDirty(const fs::path& srcPath, const fs::path& binPath) const
{
    assert(fs::is_regular_file(srcPath));

    // Check whether the expected binary exists and is newer than the source file
    if (!fs::is_regular_file(binPath)
        || fs::file_size(binPath) == 0
        || fs::last_write_time(srcPath) > fs::last_write_time(binPath))
    {
        return true;
    }

    // Check whether any included file is newer than the binary
    const auto lastBinaryUpdate = fs::last_write_time(binPath);
    for (const auto& dep : findDeps(srcPath))
    {
        if (fs::last_write_time(dep) > lastBinaryUpdate) {
            return true;
        }
    }
    return false;
}

auto ShaderLoader::findShaderSource(const util::Pathlet& filePath) const -> std::optional<fs::path>
{
    // Helper that searches all include paths for a file
    auto find = [this](const util::Pathlet& filename) -> std::optional<fs::path> {
        for (const auto& includePath : includePaths)
        {
            const auto file = includePath / filename;
            if (fs::is_regular_file(file)) {
                return file;
            }
        }
        return std::nullopt;
    };

    // Try to find the file in the shader database. This uses additional
    // information to regenerate the shader source if necessary.
    if (shaderDatabase)
    {
        if (auto shader = shaderDatabase->get(filePath.string()))
        {
            // The 'raw source' is a document that may contain unset variables
            // and from which a shader source (GLSL) is generated.
            auto rawSourcePath = find(shader->source);

            // Regenerate the shader source if it does not exist or is outdated
            auto dstFile = find(filePath);
            if (rawSourcePath && (!dstFile || binaryDirty(*rawSourcePath, *dstFile)))
            {
                log::info << "[ShaderLoader] Regenerate shader source " << filePath.string()
                          << " from " << *rawSourcePath;

                std::ifstream rawSource(*rawSourcePath);
                shader_edit::ShaderDocument doc(rawSource);
                for (const auto& [key, val] : shader->variables) {
                    doc.set(key, val);
                }

                const auto outPath = outDir / filePath;
                fs::create_directories(outPath.parent_path());
                std::ofstream outFile(outPath);
                outFile << doc.compile();

                return outPath;
            }
            else if (dstFile) {
                // File exists and is up-to-date. It might just be considered
                // up-to-date because no raw source was found for comparison.
                return *dstFile;
            }
            else {
                // File does not exist and there is no way to regenerate it
                return std::nullopt;
            }
        }
    }

    // There is no shader database, so just try to find the file. Return
    // nothing if it does not exist.
    if (auto res = find(filePath)) {
        return res;
    }

    return std::nullopt;
}

auto ShaderLoader::compile(const fs::path& srcPath, const fs::path& dstPath) const
    -> std::vector<ui32>
{
    assert(fs::is_regular_file(srcPath));

    log::info << "[ShaderLoader] Compiling shader " << srcPath << " to " << dstPath;

    auto result = spirv::generateSpirv(util::readFile(srcPath), srcPath, compileOpts);
    if (result.GetCompilationStatus()
        != shaderc_compilation_status::shaderc_compilation_status_success)
    {
        throw std::runtime_error("[In ShaderLoader::load]: Compile error when compiling shader"
                                 " source " + srcPath.string() + " to SPIRV: "
                                 + result.GetErrorMessage());
    }

    std::vector<ui32> code{ result.cbegin(), result.cend() };

    fs::create_directories(dstPath.parent_path());
    std::ofstream file(dstPath, std::ios::binary);
    file.write(reinterpret_cast<char*>(code.data()), code.size() * sizeof(ui32));

    return code;
}


ShaderLoader::ShaderDB::ShaderDB(nl::json json)
    :
    db(std::move(json))
{
}

auto ShaderLoader::ShaderDB::get(std::string_view path) const -> std::optional<ShaderInfo>
{
    auto it = db.find(path);
    if (it != db.end())
    {
        return ShaderInfo{
            .source=util::Pathlet(it->at("source").get_ref<const std::string&>()),
            .target=util::Pathlet(it->at("target").get_ref<const std::string&>()),
            .variables=it->at("variables").get<std::unordered_map<std::string, std::string>>()
        };
    }

    return std::nullopt;
}

} // namespace trc
