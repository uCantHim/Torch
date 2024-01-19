#pragma once

#include "trc/FinalLightingPass.h"
#include "trc/VulkanInclude.h"
#include "trc/core/RenderPlugin.h"
#include "trc/GBuffer.h"
#include "trc/RenderDataDescriptor.h"
#include "trc/SceneDescriptor.h"
#include "trc/ShadowPool.h"
#include "trc/AssetDescriptor.h"

namespace trc
{
    class DescriptorProviderInterface;
    class GBufferDepthReader;
    class GBufferPass;

    class RasterPlugin : public RenderPlugin
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

        RasterPlugin(const Device& device,
                     s_ptr<AssetDescriptor> assetDescriptor);

        void registerRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig& config) override;
        void registerSceneModules(SceneBase& scene) override;

        auto createRenderResources(const Device& device, const RenderTarget& target);

        void createTasks(SceneBase& scene, TaskQueue& taskQueue) override;

        //auto getGlobalDataDescriptorProvider() const -> s_ptr<const DescriptorProviderInterface>;
        //auto getSceneDescriptorProvider() const -> s_ptr<const DescriptorProviderInterface>;
        //auto getGBufferDescriptorProvider() const -> s_ptr<const DescriptorProviderInterface>;
        //auto getShadowDescriptorProvider() const -> s_ptr<const DescriptorProviderInterface>;
        //auto getAssetDescriptorProvider() const -> s_ptr<const DescriptorProviderInterface>;

        //auto getGBufferRenderPass() const -> const GBufferPass&;
        //auto getCompatibleShadowRenderPass() const -> vk::RenderPass;

    private:
        vk::UniqueRenderPass compatibleGBufferRenderPass;
        vk::UniqueRenderPass compatibleShadowRenderPass;

        s_ptr<AssetDescriptor> assetDescriptor;
        s_ptr<GBufferDescriptor> gBufferDescriptor;
        s_ptr<GlobalRenderDataDescriptor> globalDataDescriptor;
        s_ptr<SceneDescriptor> sceneDescriptor;
        s_ptr<ShadowPool> shadowPool;

        s_ptr<GBufferPass> gBufferPass;
        s_ptr<GBufferDepthReader> depthReaderPass;
        s_ptr<FinalLightingPass> finalLightingPass;
    };
} // namespace trc
