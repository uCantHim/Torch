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

        auto makeOutputConfig() -> std::unique_ptr<cloth::ShaderOutputImpl> override
        {
            return std::make_unique<DeferredFragmentShaderImpl>();
        }

        auto getBuiltins() -> BuiltinProvider& override
        {
            return builtins;
        }

        struct DeferredFragmentShaderImpl : cloth::ShaderOutputImpl
        {
            void setParameter(const std::string& outputName,
                              trc::shader::code::Value value) override
            {
                using Param = trc::FragmentModule::Parameter;
                static const std::unordered_map<std::string, Param> map{
                    { "color", Param::eColor },
                    { "normal", Param::eNormal },
                    { "specularFactor", Param::eSpecularFactor },
                    { "metallicness", Param::eMetallicness },
                    { "roughness", Param::eRoughness },
                    { "emissive", Param::eEmissive },
                };

                if (map.contains(outputName)) {
                    frag.setParameter(map.at(outputName), value);
                }
            }

            auto buildShaderOutputs(trc::shader::ShaderModuleBuilder& builder)
                -> trc::shader::ShaderOutputInterface override
            {
                const bool transparent = false;
                return frag.buildOutputs(builder, transparent);
            }

            trc::FragmentModule frag;
        };

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

        auto makeOutputConfig() -> std::unique_ptr<cloth::ShaderOutputImpl> override
        {
            return std::make_unique<ShaderImpl>(trc::VertexModuleCreateInfo{
                .animated=animated
            });
        }

        auto getBuiltins() -> BuiltinProvider& override
        {
            return builtins;
        }

        struct ShaderImpl : cloth::ShaderOutputImpl
        {
            void setParameter(const std::string& outputName,
                              trc::shader::code::Value value) override
            {
                using Param = trc::FragmentModule::Parameter;
                static const std::unordered_map<std::string, Param> map{
                    { "position", Param::eColor },
                    { "normal", Param::eNormal },
                };

                std::cout << "-- Vertex shader impl: Setting parameter \"" << outputName << "\".\n";
                //if (map.contains(outputName)) {
                //    frag.setParameter(map.at(outputName), value);
                //}
            }

            auto buildShaderOutputs(trc::shader::ShaderModuleBuilder& builder)
                -> trc::shader::ShaderOutputInterface override
            {
                return vert.buildOutputs(builder, {});
            }

            explicit ShaderImpl(const trc::VertexModuleCreateInfo& conf)
                : vert(conf)
            {}

            trc::VertexModule vert;
        };

        BuiltinProvider builtins = makeVertexBuiltinProvider();
        const bool animated;
    };
} // namespace cloth
