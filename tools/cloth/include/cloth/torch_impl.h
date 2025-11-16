#pragma once

#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>
#include <trc/material/VertexShader.h>

#include "backend_config.h"
#include "builtins.h"

namespace cloth
{
    /**
     * Uses the deferred fragment shader implementation as the default backend.
     */
    class TorchImpl : public BackendConfig
    {
    public:
        auto makeCapabilityConfig() -> std::unique_ptr<trc::shader::CapabilityConfig> override
        {
            return trc::makeFragmentCapabilityConfig();
        }

        auto makeOutputConfig() -> std::unique_ptr<trc::MaterialShaderImpl> override
        {
            return std::make_unique<trc::FragmentModule>(trc::FragmentModuleCreateInfo{
                .transparent=false,
            });
        }

        auto getBuiltins() -> BuiltinProvider& override
        {
            return builtins;
        }

        auto outputBuiltinToParameter(const FullId& out)
            -> std::optional<trc::MaterialShaderImpl::OutputParameter> override
        {
            static std::unordered_map<std::string, trc::MaterialShaderImpl::OutputParameter> map{
                { "color",          trc::FragmentModule::Out::color },
                { "normal",         trc::FragmentModule::Out::normal },
                { "specularFactor", trc::FragmentModule::Out::specularFactor },
                { "roughness",      trc::FragmentModule::Out::roughness },
                { "metallicness",   trc::FragmentModule::Out::metallicness },
                { "emissive",       trc::FragmentModule::Out::emissive },
            };
            if (map.contains(out.name)) {
                return map.at(out.name);
            }
            return std::nullopt;
        }

        BuiltinProvider builtins = makeFragmentBuiltinProvider();
    };

    class TorchVertexShaderImpl : public BackendConfig
    {
    public:
        explicit TorchVertexShaderImpl(bool animated)
            : animated(animated)
        {}

        auto makeCapabilityConfig() -> std::unique_ptr<trc::shader::CapabilityConfig> override
        {
            return trc::VertexModule::makeCapabilityConfig({ .animated=animated });
        }

        auto makeOutputConfig() -> std::unique_ptr<trc::MaterialShaderImpl> override
        {
            return std::make_unique<trc::VertexModule>(trc::VertexModuleCreateInfo{
                .animated=animated
            });
        }

        auto getBuiltins() -> BuiltinProvider& override
        {
            return builtins;
        }

        auto outputBuiltinToParameter(const FullId& out)
            -> std::optional<trc::MaterialShaderImpl::OutputParameter> override
        {
            if (out.name == "position") {
                return trc::VertexModule::Out::vertexPosition;
            }
            return std::nullopt;
        }

        BuiltinProvider builtins = makeVertexBuiltinProvider();
        const bool animated;
    };
} // namespace cloth
