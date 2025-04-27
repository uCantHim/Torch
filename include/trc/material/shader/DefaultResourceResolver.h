#pragma once

#include <unordered_map>

#include "CapabilityConfig.h"
#include "ShaderCodeCompiler.h"
#include "ShaderResourceInterface.h"

namespace trc::shader
{
    /**
     * @brief A default implementation of ResourceResolver
     *
     * Resolves capability accesses via a `ShaderCapabilityConfig` object,
     * building a `ShaderResourceInterface` object in the process.
     *
     * Combine these resource declarations with the generated code to build a
     * full shader module. This is essentially what `ShaderModuleCompiler` does.
     */
    class CapabilityConfigResourceResolver : public ResourceResolver
    {
    public:
        explicit CapabilityConfigResourceResolver(ShaderResourceInterfaceBuilder& resources)
            : resources(&resources)
        {}

        auto resolveCapabilityAccess(Capability cap) -> code::Value override {
            return resources->queryCapability(cap);
        }

        auto resolveRuntimeConstantAccess(s_ptr<ShaderRuntimeConstant> c) -> code::Value override
        {
            auto [it, success] = existingRuntimeConstants.try_emplace(c);
            if (success)
            {
                auto value = resources->makeSpecConstant(c);
                it->second = value;
                return value;
            }

            return it->second;
        }

    private:
        ShaderResourceInterfaceBuilder* resources;

        // Used to de-duplicate creations of runtime constants.
        std::unordered_map<s_ptr<ShaderRuntimeConstant>, code::Value> existingRuntimeConstants;
    };
} // namespace trc::shader
