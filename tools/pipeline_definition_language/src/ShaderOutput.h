#pragma once

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

enum class ShaderOutputType
{
    eGlsl,
    eSpirv,
};

struct ShaderInfo
{
    std::string glslCode;
    fs::path outputFileName;
    ShaderOutputType outputType;
};
