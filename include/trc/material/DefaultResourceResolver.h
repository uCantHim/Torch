#pragma once

#include <unordered_map>

#include "trc/material/ShaderCapabilityConfig.h"
#include "trc/material/ShaderCodeCompiler.h"
#include "trc/material/ShaderResourceInterface.h"

namespace trc
{
    /**
     * @brief A default implementation of ResourceResolver
     *
     * Resolves capability accesses via a `ShaderCapabilityConfig` object.
     *
     * Capability/resource queries result in entries in a `ShaderResources`
     * object. After all resources have been queried, i.e. when compilation has
     * finished, call `CapabilityConfigResourceResolver::buildResources` to get
     * a usable representation of all resources required by the shader code
     * that has been built.
     *
     * Combine these resource declarations with the generated code to build a
     * full shader module. This is essentially what `ShaderModuleCompiler` does.
     */
    class CapabilityConfigResourceResolver : public ResourceResolver
    {
    public:
        CapabilityConfigResourceResolver(
            const ShaderCapabilityConfig& conf,
            ShaderCodeBuilder& builder)
            :
            resources(conf, builder)
        {}

        auto resolveCapabilityAccess(Capability cap) -> code::Value override {
            return resources.queryCapability(cap);
        }

        auto resolveRuntimeConstantAccess(s_ptr<ShaderRuntimeConstant> c) -> code::Value override
        {
            auto [it, success] = existingRuntimeConstants.try_emplace(c);
            if (success)
            {
                auto value = resources.makeSpecConstant(c);
                it->second = value;
                return value;
            }

            return it->second;
        }

        auto buildResources() -> ShaderResources {
            return resources.compile();
        }

    private:
        ShaderResourceInterface resources;

        // Used to de-duplicate creations of runtime constants.
        std::unordered_map<s_ptr<ShaderRuntimeConstant>, code::Value> existingRuntimeConstants;
    };
} // namespace trc
