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
    inline void initRenderStages()
    {
        using namespace internal;

        RenderStage::create<DeferredStage>(RenderStages::eDeferred);
        RenderStage::create<ShadowStage>(RenderStages::eShadow);
    }

    class Renderer : public vkb::SwapchainDependentResource
    {
    public:
        Renderer();

        void drawFrame(Scene& scene, const Camera& camera);

        /**
         * TODO:
         *
         * Create a system where passes are semantically transparent to
         * the renderer and are assigned priorities. The renderer orders
         * and synchronizes the passes automatically based on that number.
         */
        void addStage(RenderStage::ID stage);

    private:
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
        std::vector<RenderStage*> renderStages;

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
        CommandCollector collector;

        vkb::DeviceLocalBuffer fullscreenQuadVertexBuffer;
    };
}
