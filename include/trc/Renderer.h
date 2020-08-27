#pragma once

#include <vkb/VulkanBase.h>
#include <vkb/Image.h>
#include <vkb/Buffer.h>
#include <vkb/FrameSpecificObject.h>

#include "Scene.h"
#include "CommandCollector.h"
#include "RenderStage.h"
#include "PipelineDefinitions.h"

#include "RenderPassShadow.h" // TODO: Rename to RenderStageShadow.h
#include "RenderStageDeferred.h"

namespace trc
{
    class Renderer : public vkb::SwapchainDependentResource
    {
    public:
        Renderer();

        void drawFrame(Scene& scene, const Camera& camera);

        void addStage(RenderStage::ID stage, ui32 priority);

    private:
        // Initialize render stages
        static inline vkb::StaticInit _render_stages_init{
            []() {
                RenderStage::create<DeferredStage>(internal::RenderStages::eDeferred);
                RenderStage::create<ShadowStage>(internal::RenderStages::eShadow);
            },
            []() {}
        };

        void signalRecreateRequired() override;
        void recreate(vkb::Swapchain& swapchain) override;
        void signalRecreateFinished() override;

        // Synchronization
        void waitForAllFrames(ui64 timeoutNs = UINT64_MAX);
        void createSemaphores();
        vkb::FrameSpecificObject<vk::UniqueSemaphore> imageAcquireSemaphores;
        vkb::FrameSpecificObject<vk::UniqueSemaphore> renderFinishedSemaphores;
        vkb::FrameSpecificObject<vk::UniqueFence> frameInFlightFences;

        // Render stages
        std::vector<std::pair<RenderStage::ID, ui32>> renderStages;
        std::vector<CommandCollector> commandCollectors;

        // General descriptor set
        void createDescriptors();
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;
        DescriptorProvider cameraDescriptorProvider{ {}, {} };

        void updateCameraMatrixBuffer(const Camera& camera);
        vkb::Buffer cameraMatrixBuffer;
        void updateGlobalDataBuffer(const vkb::Swapchain& swapchain);
        vkb::Buffer globalDataBuffer;

        // Other things
        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
    };
}
