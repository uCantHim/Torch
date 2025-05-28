#pragma once

#include <trc/material/FragmentShader.h>
#include <trc/material/TorchMaterialSettings.h>

#include "backend_config.h"

namespace cloth
{
    /**
     * Uses the deferred fragment shader implementation as the default backend.
     */
    class TorchImpl : public BackendConfig
    {
    public:
        auto makeCapabilityConfig() -> trc::shader::CapabilityConfig override
        {
            return trc::makeFragmentCapabilityConfig();
        }

        auto makeOutputConfig() -> std::unique_ptr<cloth::ShaderOutputImpl> override
        {
            return std::make_unique<DeferredFragmentShaderImpl>();
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
    };
} // namespace cloth
