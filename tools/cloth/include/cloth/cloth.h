#pragma once

#include <expected>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <trc/material/shader/CapabilityConfig.h>
#include <trc/material/shader/ShaderModuleCompiler.h>
#include <trc/material/shader/ShaderOutputInterface.h>

#include "backend_config.h"
#include "parser.h"

namespace cloth
{
    struct CompileResult
    {
        trc::shader::ShaderModule shaderModule;
    };

    struct CompileError
    {
        std::vector<parser::Error> errors;
        std::vector<std::string> initialDocumentLines;
    };

    /**
     * @brief Compile Cloth shader code to a shader module.
     *
     * @param caps    The shader input implementation.
     * @param outputs The shader output implementation.
     */
    auto compileShader(std::istream& is,
                       const trc::shader::CapabilityConfig& caps,
                       std::unique_ptr<ShaderOutputImpl> outputs)
        -> std::expected<CompileResult, CompileError>;

    /**
     * @brief Compile Cloth shader code to a shader module.
     *
     * @param caps    The shader input implementation.
     * @param outputs The shader output implementation.
     */
    auto compileShader(const parser::Result& parsedDocument,
                       const trc::shader::CapabilityConfig& caps,
                       std::unique_ptr<ShaderOutputImpl> outputs)
        -> std::expected<CompileResult, CompileError>;

    /**
     * @brief Print compile errors in a nicely formatted way.
     *
     * @param filePath The path to the cloth file in which the errors occurred.
     *                 Is included as information in the generated error message
     *                 if present.
     */
    auto printErrors(const cloth::CompileError& doc, std::optional<std::string> filePath)
        -> std::string;
} // namespace cloth
