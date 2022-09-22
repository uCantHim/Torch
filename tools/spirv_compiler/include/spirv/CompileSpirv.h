#pragma once

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

#include <shaderc/shaderc.hpp>

namespace spirv
{
    auto generateSpirv(const std::string& code,
                       const fs::path& inputFilePath,
                       const shaderc::CompileOptions& opts = {})
        -> shaderc::SpvCompilationResult;
} // namespace spirv
