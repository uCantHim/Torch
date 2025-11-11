#pragma once

#include <memory>

#include <trc/material/shader/CapabilityConfig.h>
#include <trc/material/shader/ShaderModuleBuilder.h>
#include <trc/material/shader/ShaderOutputInterface.h>

namespace cloth
{
    class BuiltinProvider;

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

    class BackendConfig
    {
    public:
        virtual ~BackendConfig() noexcept = default;

        virtual auto getBuiltins() -> BuiltinProvider& = 0;
        virtual auto makeBuilder() -> std::unique_ptr<trc::shader::ShaderModuleBuilder> = 0;
        virtual auto makeOutputConfig() -> std::unique_ptr<ShaderOutputImpl> = 0;
    };
} // namespace cloth
