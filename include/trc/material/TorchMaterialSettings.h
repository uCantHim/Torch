#pragma once

#include <optional>
#include <string>
#include <vector>

#include "trc/assets/AssetReference.h"
#include "trc/assets/Texture.h"
#include "trc/material/MaterialShaderProgram.h"
#include "trc/material/ShaderCapabilityConfig.h"

namespace trc
{
    /**
     * @brief Create an object that defines all inputs that Torch provides to
     *        shaders
     *
     * @return ShaderCapabilityConfig A configuration for fragment shaders in
     *                                Torch's material system for standard
     *                                drawable objects.
     */
    auto makeFragmentCapabilityConfig() -> ShaderCapabilityConfig;

    /**
     * @brief Create a configuration that implements all capabilities that Torch
     *        provides to shaders in the `MaterialCapability` namespace.
     *
     * Also implements all capabilities in `RayHitCapability` for internal
     * use.
     *
     * @return ShaderCapabilityConfig A configuration for callable shaders in
     *                                Torch's material system for standard
     *                                drawable objects.
     */
    auto makeRayHitCapabilityConfig() -> ShaderCapabilityConfig;

    /**
     * @brief Create an object that defines all possibly existing descriptors
     */
    auto makeShaderDescriptorConfig() -> ShaderDescriptorConfig;

    /**
     * @brief A specialization constant that specifies a texture's device index
     */
    class RuntimeTextureIndex : public ShaderRuntimeConstant
    {
    public:
        explicit RuntimeTextureIndex(AssetReference<Texture> texture);

        auto loadData() -> std::vector<std::byte> override;
        auto serialize() const -> std::string override;

    private:
        AssetReference<Texture> texture;
        std::optional<AssetHandle<Texture>> runtimeHandle;
    };
} // namespace trc
