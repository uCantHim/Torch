#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
namespace fs = std::filesystem;

#include <gtest/gtest.h>

#include <spirv/CompileSpirv.h>
#include <trc/ShaderLoader.h>
#include <trc/base/ShaderProgram.h>
#include <trc_util/Util.h>

const fs::path datadir = DATADIR / fs::path{ "test_shader_loader" };

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

    shaderc::CompileOptions opts = trc::ShaderLoader::makeDefaultOptions();
    auto truthRes = spirv::generateSpirv(
        trc::util::readFile(datadir / "test.vert"),
        datadir / "test.vert",
        opts
    );
    const std::vector<uint32_t> truth(truthRes.begin(), truthRes.end());

    ASSERT_EQ(code, truth);
    ASSERT_EQ(trc::readSpirvFile(shaderBinaryFile), truth);

    // Compiled shader is cached and not compiled a second time
    const auto cacheTime = std::chrono::file_clock::now();
    loader.load(trc::ShaderPath("test.vert"));
    ASSERT_FALSE(fs::last_write_time(shaderBinaryFile) > cacheTime);

    // Code *is* compiled again after modification
    fs::last_write_time(shaderBinaryFile, std::chrono::file_clock::now());
    loader.load(trc::ShaderPath("test.vert"));
    ASSERT_TRUE(fs::last_write_time(shaderBinaryFile) > cacheTime);
}
