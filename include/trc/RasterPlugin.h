#pragma once

#include "trc/AssetDescriptor.h"
#include "trc/FinalLighting.h"
#include "trc/GBuffer.h"
#include "trc/GBufferDepthReader.h"
#include "trc/GBufferPass.h"
#include "trc/RenderDataDescriptor.h"
#include "trc/SceneDescriptor.h"
#include "trc/ShadowPool.h"
#include "trc/VulkanInclude.h"
#include "trc/core/RenderPlugin.h"

namespace trc
{
    class DescriptorProviderInterface;
    class GBufferDepthReader;
    class GBufferPass;

    struct RasterPluginCreateInfo
    {
        // The instance that makes an AssetRegistry's data available to the
        // device. Create this descriptor, which contains information about
        // Torch's default assets, via `makeDefaultAssetModules`. This function
        // registers all asset modules that are necessary to use Torch's default
        // assets at an asset registry and builds a descriptor for their data.
        //
        // The same asset descriptor can be used for multiple render
        // configurations.
        s_ptr<AssetDescriptor> assetDescriptor;

        s_ptr<ShadowPool> shadowDescriptor;

        ui32 maxTransparentFragsPerPixel{ 3 };
        bool enableRayTracing{ false };

        // A function that returns the current mouse position. Used to read the
        // depth value at the current mouse position for use in
        // `TorchRenderConfig::getMouseWorldPos`.
        std::function<vec2()> mousePosGetter{ []{ return vec2(0.0f); } };
    };

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
                     ui32 maxViewports,
                     const RasterPluginCreateInfo& createInfo);

        void registerRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig& config) override;
        void registerSceneModules(SceneBase& scene) override;

        auto createDrawConfig(const Device& device, Viewport renderTarget)
            -> u_ptr<DrawConfig> override;

    private:
        class RasterDrawConfig : public DrawConfig
        {
        public:
            RasterDrawConfig(const Device& device, Viewport renderTarget, RasterPlugin& parent);

            void registerResources(ResourceStorage& resources) override;

            void update(SceneBase& scene, const Camera& camera) override;
            void createTasks(SceneBase& scene, TaskQueue& taskQueue) override;

        private:
            RasterPlugin* parent;

            GBuffer gBuffer;
            s_ptr<GBufferPass> gBufferPass;
            s_ptr<GBufferDepthReader> depthReaderPass;
            u_ptr<FinalLightingDispatcher> finalLighting;

            vk::UniqueDescriptorSet gBufferDescSet;
            s_ptr<GlobalRenderDataDescriptor::DescriptorSet> globalDataDescriptor;
        };

        /**
         * Called once in `defineResources`.
         */
        void _createFinalLightingResources(ResourceConfig& resources);

        vk::UniqueRenderPass compatibleGBufferRenderPass;
        vk::UniqueRenderPass compatibleShadowRenderPass;

        s_ptr<AssetDescriptor> assetDescriptor;
        GBufferDescriptor gBufferDescriptor;
        GlobalRenderDataDescriptor globalDataDescriptor;
        s_ptr<SceneDescriptor> sceneDescriptor;
        s_ptr<ShadowPool> shadowDescriptor;

        FinalLighting finalLighting;
    };
} // namespace trc
