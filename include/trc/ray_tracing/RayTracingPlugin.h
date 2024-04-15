#pragma once

#include "trc/TopLevelAccelerationStructureBuilder.h"
#include "trc/core/RenderPlugin.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/FinalCompositingPass.h"
#include "trc/ray_tracing/RayTracingPass.h"

namespace trc
{
    class RayTracingPlugin : public RenderPlugin
    {
    public:
        /**
         * Contains a TLAS and an output image.
         */
        static constexpr auto RAYGEN_DESCRIPTOR{ "trc_raytracing_raygen_descriptor" };

        RayTracingPlugin(const Instance& instance,
                         ResourceConfig& resourceConfig,
                         ui32 maxViewports,
                         ui32 tlasMaxInstances);

        void registerRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig& conf) override;

        auto createDrawConfig(const Device& device, Viewport renderTarget)
            -> u_ptr<DrawConfig> override;

    private:
        class RayDrawConfig : public DrawConfig
        {
        public:
            RayDrawConfig(RayTracingPlugin& parent, Viewport renderTarget);

            void registerResources(ResourceStorage& resources) override;

            void update(const Device& device, SceneBase& scene, const Camera& camera) override;
            void createTasks(SceneBase& scene, TaskQueue& taskQueue) override;

        private:
            rt::RayBuffer rayBuffer;
            rt::TLAS tlas;
            vk::UniqueDescriptorSet raygenDescriptor;

            TopLevelAccelerationStructureBuilder tlasBuilder;
            rt::FinalCompositingPass compositingPass;

            RayTracingCall reflectionsRayCall;
        };

        static constexpr ui32 kMaxRecursionDepth{ 16 };

        const ui32 maxTlasInstances;
        const Instance& instance;

        rt::RaygenDescriptorPool raygenDescriptorPool;
        rt::CompositingDescriptor compositingDescriptorPool;

        PipelineLayout reflectPipelineLayout;
        MemoryPool sbtMemoryPool;
        s_ptr<Pipeline> reflectPipeline;
        u_ptr<rt::ShaderBindingTable> reflectShaderBindingTable;
    };
} // namespace trc
