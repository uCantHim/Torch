#pragma once

#include <unordered_map>

#include "trc/FinalLighting.h"
#include "trc/GBuffer.h"
#include "trc/GBufferDepthReader.h"
#include "trc/GBufferPass.h"
#include "trc/RenderDataDescriptor.h"
#include "trc/ShadowPool.h"
#include "trc/ShadowRegistry.h"
#include "trc/core/RenderPlugin.h"

namespace trc
{
    class DescriptorProviderInterface;
    class GBufferPass;
    class SceneDescriptor;

    struct RasterPluginCreateInfo
    {
        /**
         * The maximum number of shadow maps that may be used for lighting.
         */
        ui32 maxShadowMaps{ 100 };

        /**
         * A maximum number of transparent objects that may overlap on a single
         * pixel.
         *
         * This does not actually limit the possible number of transparent
         * fragments that are blended together per pixel, but is a heuristic
         * from which an upper bound on the number of transparent fragments on
         * the entire viewport is calculated.
         */
        ui32 maxTransparentFragsPerPixel{ 3 };

        /**
         * An interface to the plugin's capability to read depth values from
         * the g-buffer.
         *
         * Passing `nullptr` disables this feature.
         */
        s_ptr<DepthReaderCallback> depthReaderCallback{ nullptr };
    };

    auto buildRasterPlugin(const RasterPluginCreateInfo& createInfo) -> PluginBuilder;

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

        void defineRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig& config) override;

        auto createSceneResources(SceneContext& ctx)
            -> u_ptr<SceneResources> override;
        auto createViewportResources(ViewportContext& ctx)
            -> u_ptr<ViewportResources> override;

    private:
        class DrawConfig : public ViewportResources
        {
        public:
            DrawConfig(ViewportContext& ctx, RasterPlugin& parent);

            void registerResources(ResourceStorage& resources) override;

            void hostUpdate(ViewportContext& ctx) override;
            void createTasks(ViewportDrawTaskQueue& taskQueue, ViewportContext& ctx) override;

        private:
            RasterPlugin* parent;

            GBuffer gBuffer;
            s_ptr<GBufferPass> gBufferPass;
            s_ptr<GBufferDepthReader> gBufferDepthReaderPass;
            u_ptr<FinalLightingDispatcher> finalLighting;

            vk::UniqueDescriptorSet gBufferDescSet;
            s_ptr<GlobalRenderDataDescriptor::DescriptorSet> globalDataDescriptor;
        };

        class SceneConfig : public SceneResources
        {
        public:
            SceneConfig(SceneContext& ctx, RasterPlugin& parent);

            void registerResources(ResourceStorage& resources) override;

            void hostUpdate(SceneContext& ctx) override;
            void createTasks(SceneUpdateTaskQueue& taskQueue) override;

        private:
            ShadowPool shadowPool;
            s_ptr<ShadowDescriptor::DescriptorSet> shadowDescriptorSet;

            std::unordered_map<ShadowID, s_ptr<ShadowMap>> allocatedShadows;

            // Keep our shadow event notifiers alive:
            EventListener<ShadowRegistry::ShadowCreateEvent> onShadowCreate;
            EventListener<ShadowRegistry::ShadowDestroyEvent> onShadowDestroy;
        };

        /**
         * Called once in `defineResources`.
         */
        void createFinalLightingResources(ResourceConfig& resources);

        const RasterPluginCreateInfo config;

        vk::UniqueRenderPass compatibleGBufferRenderPass;
        vk::UniqueRenderPass compatibleShadowRenderPass;

        GBufferDescriptor gBufferDescriptor;
        GlobalRenderDataDescriptor globalDataDescriptor;
        s_ptr<SceneDescriptor> sceneDescriptor;
        ShadowDescriptor shadowDescriptor;

        FinalLighting finalLighting;
    };
} // namespace trc
