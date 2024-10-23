#pragma once

#include "trc/TopLevelAccelerationStructureBuilder.h"
#include "trc/base/MemoryPool.h"
#include "trc/core/RenderPlugin.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/FinalCompositingPass.h"
#include "trc/ray_tracing/RayTracingPass.h"
#include "trc/ray_tracing/RaygenDescriptor.h"

namespace trc
{
    struct RayTracingPluginCreateInfo
    {
        /**
         * The maximum number of individual geometric objects that may be ray
         * traced at any point.
         *
         * Must be geater than 0.
         *
         * TODO: Dynamically resize the buffers in the plugin's scene resources.
         */
        ui32 maxTlasInstances;
    };

    auto buildRayTracingPlugin(const RayTracingPluginCreateInfo& createInfo) -> PluginBuilder;

    class RayTracingPlugin : public RenderPlugin
    {
    public:
        /**
         * Contains a TLAS and an output image.
         */
        static constexpr auto RAYGEN_TLAS_DESCRIPTOR{ "trc_raytracing_raygen_tlas_descriptor" };
        static constexpr auto RAYGEN_IMAGE_DESCRIPTOR{ "trc_raytracing_raygen_image_descriptor" };

        RayTracingPlugin(const Instance& instance,
                         ui32 maxViewports,
                         const RayTracingPluginCreateInfo& createInfo);

        void defineRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig& conf) override;

        auto createSceneResources(SceneContext& ctx)
            -> u_ptr<SceneResources> override;

        auto createViewportResources(ViewportContext& ctx)
            -> u_ptr<ViewportResources> override;

    private:
        class TlasUpdateConfig : public SceneResources
        {
        public:
            TlasUpdateConfig(RayTracingPlugin& parent);

            void registerResources(ResourceStorage& resources) override;
            void hostUpdate(SceneContext& ctx) override;
            void createTasks(SceneUpdateTaskQueue& taskQueue) override;

        private:
            rt::TLAS tlas;
            TopLevelAccelerationStructureBuilder tlasBuilder;

            vk::UniqueDescriptorSet tlasDescriptor;
        };

        class RayDrawConfig : public ViewportResources
        {
        public:
            RayDrawConfig(RayTracingPlugin& parent, Viewport renderTarget);

            void registerResources(ResourceStorage& resources) override;
            void hostUpdate(ViewportContext&) override {}
            void createTasks(ViewportDrawTaskQueue& taskQueue, ViewportContext&) override;

        private:
            rt::RayBuffer rayBuffer;
            vk::UniqueDescriptorSet outputImageDescriptor;

            rt::FinalCompositingDispatcher compositingPass;
            RayTracingCall reflectionsRayCall;
        };

        void init(ViewportContext& ctx);

        static constexpr ui32 kMaxRecursionDepth{ 16 };

        const ui32 maxTlasInstances;
        const Instance& instance;

        bool isInitialized{ false };

        rt::RaygenDescriptorPool raygenDescriptorPool;
        rt::CompositingDescriptor compositingDescriptorPool;

        u_ptr<PipelineLayout> reflectPipelineLayout;
        MemoryPool sbtMemoryPool;
        s_ptr<Pipeline> reflectPipeline;
        u_ptr<rt::ShaderBindingTable> reflectShaderBindingTable;
    };
} // namespace trc
