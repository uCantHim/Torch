#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
namespace fs = std::filesystem;

#include <gtest/gtest.h>

#include <spirv/CompileSpirv.h>
#include <trc/ShaderLoader.h>

const fs::path datadir = DATADIR / fs::path{ "test_shader_loader" };

auto read(const fs::path& path) -> std::string
{
    std::ifstream file(path);
    std::stringstream result;
    result << file.rdbuf();

    return result.str();
}

TEST(TestShaderLoader, AutomaticRecompilation)
{
    const fs::path shaderBinaryDir = datadir / "shader_bins";
    fs::create_directory(shaderBinaryDir);
    const fs::path shaderBinaryFile = shaderBinaryDir / "test.vert.spv";

    const auto testStart = std::chrono::file_clock::now();

    trc::ShaderLoader loader({ datadir }, shaderBinaryDir);

    const auto code = loader.load(trc::ShaderPath("test.vert"));
    ASSERT_TRUE(fs::is_regular_file(shaderBinaryFile));
    ASSERT_TRUE(fs::last_write_time(shaderBinaryFile) > fs::last_write_time(datadir / "test.vert"));

    shaderc::CompileOptions opts;
#ifdef TRC_FLIP_Y_PROJECTION
    opts.AddMacroDefinition("TRC_FLIP_Y_AXIS");
#endif
    opts.SetTargetSpirv(shaderc_spirv_version_1_5);
    opts.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan,
                              shaderc_env_version_vulkan_1_3);
    opts.SetOptimizationLevel(shaderc_optimization_level_performance);
    auto truthRes = spirv::generateSpirv(read(datadir / "test.vert"), datadir / "test.vert", opts);
    std::string truth((char*)truthRes.begin(), (truthRes.end() - truthRes.begin()) * sizeof(uint32_t));

    ASSERT_EQ(code, truth);
    ASSERT_EQ(read(shaderBinaryFile), truth);

    // Compiled shader is cached and not compiled a second time
    const auto cacheTime = std::chrono::file_clock::now();
    loader.load(trc::ShaderPath("test.vert"));
    ASSERT_FALSE(fs::last_write_time(shaderBinaryFile) > cacheTime);

    // Code *is* compiled again after modification
    fs::last_write_time(shaderBinaryFile, std::chrono::file_clock::now());
    loader.load(trc::ShaderPath("test.vert"));
    ASSERT_TRUE(fs::last_write_time(shaderBinaryFile) > cacheTime);
}
