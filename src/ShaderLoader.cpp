#include "trc/ShaderLoader.h"

#include <fstream>
#include <iostream>

#include <vkb/ShaderProgram.h>
#include <spirv/CompileSpirv.h>
#include <spirv/FileIncluder.h>

#include "trc/Types.h"
#include "trc/util/TorchDirectories.h"

namespace nl = nlohmann;



namespace trc
{

ShaderLoader::ShaderLoader(
    std::vector<fs::path> includePaths,
    fs::path binaryPath)
    :
    includePaths(std::move(includePaths)),
    outDir(std::move(binaryPath))
{
    if (fs::exists(outDir) && !fs::is_directory(outDir)) {
        throw std::invalid_argument("[In ShaderLoader::ShaderLoader]: Object at binary directory"
                                    " path " + outDir.string() + " exists but is not a directory!");
    }
    fs::create_directories(outDir);
}

auto ShaderLoader::load(ShaderPath shaderPath) -> std::string
{
    for (const auto& includePath : includePaths)
    {
        auto res = tryLoad(includePath, shaderPath);
        if (res) return *res;
    }

    throw std::out_of_range("[In ShaderLoader::load]: Shader source "
                            + shaderPath.getSourceName().string() + " not found.");
}

auto ShaderLoader::tryLoad(const fs::path& includeDir, const ShaderPath& shaderPath)
    -> std::optional<std::string>
{
    const auto srcPath = includeDir / shaderPath.getSourceName();
    const auto binPath = outDir / shaderPath.getBinaryName();
    if (!fs::is_regular_file(srcPath)) {
        return std::nullopt;
    }

    if (!fs::is_regular_file(binPath)
        || fs::last_write_time(srcPath) > fs::last_write_time(binPath))
    {
        return compile(srcPath, binPath);
    }

    return vkb::readFile(binPath);
}

auto ShaderLoader::compile(const fs::path& srcPath, const fs::path& dstPath) -> std::string
{
    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Compiling shader " << srcPath << " to " << dstPath << "\n";
    }

    shaderc::CompileOptions opts;
#ifdef TRC_FLIP_Y_PROJECTION
    opts.AddMacroDefinition("TRC_FLIP_Y_AXIS");
#endif
    opts.SetTargetSpirv(shaderc_spirv_version_1_5);
    opts.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan,
                              shaderc_env_version_vulkan_1_3);
    opts.SetIncluder(std::make_unique<spirv::FileIncluder>(srcPath.parent_path()));
    opts.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto result = spirv::generateSpirv(vkb::readFile(srcPath), srcPath, opts);
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

    std::ofstream file(dstPath, std::ios::binary);
    file << code;

    return code;
}

} // namespace trc
