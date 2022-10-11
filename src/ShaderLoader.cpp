#include "trc/ShaderLoader.h"

#include <cstring>

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include <spirv/FileIncluder.h>
#include <shader_tools/ShaderDocument.h>

#include "trc/Types.h"
#include "trc/base/ShaderProgram.h"
#include "trc/util/TorchDirectories.h"

namespace nl = nlohmann;



namespace trc
{

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

    compileOpts.SetIncluder(std::make_unique<spirv::FileIncluder>(
        includePaths.front(),
        std::vector<fs::path>{ includePaths.begin() + 1, includePaths.end() }
    ));
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

auto ShaderLoader::load(ShaderPath shaderPath) -> std::string
{
    /**
     * The longest possible dependency chain is:
     *
     *     file  -->  GLSL source  -->  SPIRV code
     *
     * where `file` contains unset variables. This intermediate 'true' GLSL
     * source is generated in `findFile` if it is outdated.
     */

    const auto srcPathOpt = findFile(shaderPath.getSourceName());
    if (srcPathOpt)
    {
        assert(fs::is_regular_file(*srcPathOpt));

        const auto& srcPath = *srcPathOpt;
        const auto binPath = outDir / shaderPath.getBinaryName();

        if (binaryDirty(srcPath, binPath))
        {
            return compile(srcPath, binPath);
        }

        return readFile(binPath);
    }

    throw std::out_of_range("[In ShaderLoader::load]: Shader source "
                            + shaderPath.getSourceName().string() + " not found.");
}

bool ShaderLoader::binaryDirty(const fs::path& srcPath, const fs::path& binPath)
{
    return !fs::is_regular_file(binPath)
        || fs::last_write_time(srcPath) > fs::last_write_time(binPath)
        || fs::file_size(binPath) == 0;
}

auto ShaderLoader::findFile(const util::Pathlet& filePath) const -> std::optional<fs::path>
{
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

    if (auto res = find(filePath)) {
        return res;
    }

    // Source file not found - try to find a raw pre-variable-replacement version
    if (shaderDatabase)
    {
        if (auto shader = shaderDatabase->get(filePath.string()))
        {
            auto rawSourcePath = find(shader->source);
            if (!rawSourcePath) {
                return std::nullopt;
            }

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
    }

    return std::nullopt;
}

auto ShaderLoader::compile(const fs::path& srcPath, const fs::path& dstPath) -> std::string
{
    assert(fs::is_regular_file(srcPath));

    if constexpr (enableVerboseLogging) {
        std::cout << "Compiling shader " << srcPath << " to " << dstPath << "\n";
    }

    auto result = spirv::generateSpirv(readFile(srcPath), srcPath, compileOpts);
    if (result.GetCompilationStatus()
        != shaderc_compilation_status::shaderc_compilation_status_success)
    {
        throw std::runtime_error("[In ShaderLoader::load]: Compile error when compiling shader"
                                 " source " + srcPath.string() + " to SPIRV: "
                                 + result.GetErrorMessage());
    }

    std::string code(
        reinterpret_cast<const char*>(result.begin()),
        static_cast<std::streamsize>(
            (result.end() - result.begin()) * sizeof(decltype(result)::element_type)
        )
    );

    fs::create_directories(dstPath.parent_path());
    std::ofstream file(dstPath, std::ios::binary);
    file << code;

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
