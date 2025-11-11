#pragma once

#include <expected>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
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
                       BackendConfig& impl)
        -> std::expected<CompileResult, CompileError>;

    /**
     * @brief Compile Cloth shader code to a shader module.
     *
     * @param caps    The shader input implementation.
     * @param outputs The shader output implementation.
     */
    auto compileShader(const parser::Result& parsedDocument,
                       BackendConfig& impl)
        -> std::expected<CompileResult, CompileError>;

    struct MultiCompileResult
    {
        // Successfully compiled shader stages.
        std::unordered_map<parser::ShaderStage, CompileResult> shaderStages;

        // Shader stages that failed to compile.
        std::unordered_map<parser::ShaderStage, CompileError> shaderStageErrors;

        // A list of additional errors related to parsing the shader block
        // declarations.
        std::vector<parser::Error> errors;

        // A list of warnings issued by the compiler.
        std::vector<parser::Error> warnings;

        bool hasErrors() const
        {
            return !errors.empty() || !shaderStageErrors.empty();
        }
    };

    auto compileMultiShader(
        std::istream& is,
        const std::unordered_map<parser::ShaderStage, std::shared_ptr<BackendConfig>>& impl
        ) -> MultiCompileResult;

    auto compileMultiShader(
        const parser::MultiDocumentResult& parsedDocument,
        const std::unordered_map<parser::ShaderStage, std::shared_ptr<BackendConfig>>& impl
        ) -> MultiCompileResult;

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
