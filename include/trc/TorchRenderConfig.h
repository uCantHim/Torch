#pragma once

#include <vkb/event/Event.h>

#include "Types.h"
#include "core/Instance.h"
#include "core/RenderConfigCrtpBase.h"
#include "core/RenderPass.h"
#include "core/SceneBase.h"
#include "core/RenderGraph.h"
#include "core/DescriptorProviderWrapper.h"

#include "GBufferPass.h"
#include "RenderPassShadow.h"
#include "FinalLightingPass.h"
#include "RenderDataDescriptor.h"
#include "SceneDescriptor.h"
#include "ShadowPool.h"
#include "AssetRegistry.h"
#include "AnimationDataStorage.h"
#include "text/FontDataStorage.h"

namespace trc
{
    class Window;

    /**
     * @brief
     */
    struct TorchRenderConfigCreateInfo
    {
        RenderGraph renderGraph;
        const RenderTarget& target;

        AssetRegistry* assetRegistry;
        ShadowPool* shadowPool;

        ui32 maxTransparentFragsPerPixel{ 3 };
    };

    auto makeDeferredRenderGraph() -> RenderGraph;

    /**
     * @brief
     */
    class TorchRenderConfig : public RenderConfigCrtpBase<TorchRenderConfig>
    {
    public:
        /**
         * Camera matrices, resolution, mouse position
         */
        static constexpr auto GLOBAL_DATA_DESCRIPTOR{ "global_data" };

        /**
         * All of the asset registry's data
         */
        static constexpr auto ASSET_DESCRIPTOR{ "asset_registry" };

        /**
         * Keyframe transforms
         */
        static constexpr auto ANIMATION_DESCRIPTOR{ "animation_data" };

        /**
         * Font bitmaps
         */
        static constexpr auto FONT_DESCRIPTOR{ "fonts" };

        /**
         * Lights
         */
        static constexpr auto SCENE_DESCRIPTOR{ "scene_data" };

        /**
         * Storage images, transparency buffer, swapchain image
         */
        static constexpr auto G_BUFFER_DESCRIPTOR{ "g_buffer" };

        /**
         * Shadow matrices, shadow maps
         */
        static constexpr auto SHADOW_DESCRIPTOR{ "shadow" };

        static constexpr auto OPAQUE_G_BUFFER_PASS{ "g_buffer" };
        static constexpr auto TRANSPARENT_G_BUFFER_PASS{ "transparency" };
        static constexpr auto SHADOW_PASS{ "shadow" };
        static constexpr auto FINAL_LIGHTING_PASS{ "final_lighting" };

        /**
         * @brief
         */
        TorchRenderConfig(const Window& window, const TorchRenderConfigCreateInfo& info);

        void preDraw(const DrawConfig& draw) override;
        void postDraw(const DrawConfig& draw) override;

        void setViewport(uvec2 offset, uvec2 size) override;
        void setRenderTarget(const RenderTarget& newTarget) override;

        void setClearColor(vec4 color);

        auto getGBuffer() -> vkb::FrameSpecific<GBuffer>&;
        auto getGBuffer() const -> const vkb::FrameSpecific<GBuffer>&;

        auto getGBufferRenderPass() const -> const GBufferPass&;
        auto getCompatibleShadowRenderPass() const -> vk::RenderPass;

        auto getGlobalDataDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getSceneDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getGBufferDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getShadowDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getAssetDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getFontDescriptorProvider() const -> const DescriptorProviderInterface&;
        auto getAnimationDataDescriptorProvider() const -> const DescriptorProviderInterface&;

        auto getAssets() -> AssetRegistry&;
        auto getAssets() const -> const AssetRegistry&;
        auto getShadowPool() -> ShadowPool&;
        auto getShadowPool() const -> const ShadowPool&;

        auto getMouseDepth() const -> float;
        auto getMousePosAtDepth(const Camera& camera, float depth) const -> vec3;
        auto getMouseWorldPos(const Camera& camera) const -> vec3;

    private:
        void createGBuffer(uvec2 newSize);

        const Window& window;

        // Default render passes
        u_ptr<vkb::FrameSpecific<GBuffer>> gBuffer;
        u_ptr<GBufferPass> gBufferPass;
        RenderPassShadow shadowPass;
        u_ptr<FinalLightingPass> finalLightingPass;

        // Descriptors
        GBufferDescriptor gBufferDescriptor;
        GlobalRenderDataDescriptor globalDataDescriptor;
        SceneDescriptor sceneDescriptor;

        DescriptorProvider fontDataDescriptor;

        // Data & Assets
        AssetRegistry* assetRegistry;
        ShadowPool* shadowPool;
    };
} // namespace trc
