#pragma once

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

#include <shaderc/shaderc.hpp>

namespace spirv
{
    /**
     * @param code          GLSL code.
     * @param inputFilePath Path of the GLSL file in which the code resides.
     * @param opts          Compile options.
     * @param shaderKind    (optional) Type of shader to compile. If not
     *                      specified, the shader type is inferred from the
     *                      file extension. If specified, this overrides any
     *                      deduction from file extension.
     */
    auto generateSpirv(const std::string& code,
                       const fs::path& inputFilePath,
                       const shaderc::CompileOptions& opts = {},
                       std::optional<shaderc_shader_kind> shaderKind = {})
        -> shaderc::SpvCompilationResult;
} // namespace spirv
