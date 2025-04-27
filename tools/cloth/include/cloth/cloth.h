#pragma once

#include <generator>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <trc/material/shader/CapabilityConfig.h>
#include <trc/material/shader/ShaderModuleCompiler.h>
#include <trc/material/shader/ShaderOutputInterface.h>

namespace cloth
{
    struct ShaderOutputImpl
    {
        virtual ~ShaderOutputImpl() noexcept = default;

        void defineParameter(const std::string& name, trc::shader::BasicType type);

        auto getParameters() const
            -> std::generator<std::pair<std::string_view, trc::shader::BasicType>>;
        auto getParameterType(const std::string& param) const
            -> std::optional<trc::shader::BasicType>;

        virtual void setParameter(const std::string& param, trc::shader::code::Value value);

        virtual auto buildShaderOutputs(trc::shader::ShaderModuleBuilder& builder)
            -> trc::shader::ShaderOutputInterface = 0;

    protected:
        auto getParamValues() const
            -> std::generator<std::pair<std::string_view, trc::shader::code::Value>>;

    private:
        std::unordered_map<std::string, trc::shader::BasicType> params;
        std::unordered_map<std::string, trc::shader::code::Value> paramValues;
    };

    // TODO: Error handling
    struct CompileResult {
        trc::shader::ShaderModule shaderModule;
    };

    /**
     * @brief Compile Cloth shader code to a shader module.
     */
    auto compileShader(std::istream& is,
                       trc::shader::CapabilityConfig& caps,
                       ShaderOutputImpl& outputs)
        -> CompileResult;
} // namespace cloth
