#pragma once

#include "trc/AssetDescriptor.h"
#include "trc/FinalLighting.h"
#include "trc/GBuffer.h"
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
        s_ptr<ShadowPool> shadowDescriptor;

        /**
         * This does not actually limit the possible number of transparent
         * fragments that are blended together per pixel, but is a heuristic
         * from which an upper bound on the number of transparent fragments on
         * the entire viewport is calculated.
         */
        ui32 maxTransparentFragsPerPixel{ 3 };
    };

    class RasterPlugin : public RenderPlugin
    {
    public:
        /**
         * Camera matrices, resolution, mouse position
         */
        static constexpr auto GLOBAL_DATA_DESCRIPTOR{ "global_data" };

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

            void update(const Device& device, SceneBase& scene, const Camera& camera) override;
            void createTasks(SceneBase& scene, TaskQueue& taskQueue) override;

        private:
            RasterPlugin* parent;

            GBuffer gBuffer;
            s_ptr<GBufferPass> gBufferPass;
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

        GBufferDescriptor gBufferDescriptor;
        GlobalRenderDataDescriptor globalDataDescriptor;
        s_ptr<SceneDescriptor> sceneDescriptor;
        s_ptr<ShadowPool> shadowDescriptor;

        FinalLighting finalLighting;
    };
} // namespace trc
