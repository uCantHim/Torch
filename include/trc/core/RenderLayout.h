#pragma once

#include <vector>

#include <vkb/basics/Device.h>
#include <vkb/FrameSpecificObject.h>

#include "RenderStage.h"

namespace trc
{
    class Window;
    class RenderPass;
    class RenderGraph;
    struct DrawConfig;

    /**
     * @brief A command collection policy
     *
     * Defines the structure of a complete image synthesis pathway. Can
     * declare stages in which one or more render passes can be executed.
     */
    class RenderLayout
    {
    public:
        RenderLayout(const RenderLayout&) = delete;
        auto operator=(const RenderLayout&) -> RenderLayout& = delete;

        explicit RenderLayout(const Window& window, const RenderGraph& graph);
        RenderLayout(RenderLayout&&) noexcept = default;
        ~RenderLayout() = default;

        auto operator=(RenderLayout&&) noexcept -> RenderLayout& = default;

        auto record(const DrawConfig& draw) -> std::vector<vk::CommandBuffer>;

        void addPass(RenderStageType::ID stage, RenderPass& newPass);
        void removePass(RenderStageType::ID stage, RenderPass& pass);

    private:
        struct Stage
        {
            RenderStageType::ID id;
            std::vector<RenderPass*> renderPasses;

            std::vector<vk::Event> signalEvents;
            std::vector<vk::Event> waitEvents;
        };

        void recordStage(vk::CommandBuffer cmdBuf, const DrawConfig& draw, const Stage& stage);
        auto getStage(RenderStageType::ID stage) -> Stage*;

        std::vector<Stage> stages;

        std::vector<vk::UniqueEvent> events;
        std::vector<vk::UniqueCommandPool> commandPools;
        std::vector<vkb::FrameSpecific<vk::UniqueCommandBuffer>> commandBuffers;
    };
} // namespace trc