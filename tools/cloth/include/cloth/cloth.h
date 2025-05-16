#pragma once

#include <expected>
#include <iosfwd>
#include <string>
#include <vector>

#include <trc/material/shader/CapabilityConfig.h>
#include <trc/material/shader/ShaderModuleCompiler.h>
#include <trc/material/shader/ShaderOutputInterface.h>

#include "parser.h"

namespace cloth
{
    /**
     * Implements shader outputs in the form of semantical parameters for an
     * engine backend.
     */
    struct ShaderOutputImpl
    {
        virtual ~ShaderOutputImpl() noexcept = default;

        virtual void setParameter(const std::string& param, trc::shader::code::Value value) = 0;
        virtual auto buildShaderOutputs(trc::shader::ShaderModuleBuilder& builder)
            -> trc::shader::ShaderOutputInterface = 0;
    };

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
                       trc::shader::CapabilityConfig& caps,
                       ShaderOutputImpl& outputs)
        -> std::expected<CompileResult, CompileError>;
} // namespace cloth
