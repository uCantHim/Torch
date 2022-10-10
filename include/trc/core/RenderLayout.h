#pragma once

#include <vector>

#include "trc/base/Device.h"
#include "trc/base/FrameSpecificObject.h"
#include <trc_util/async/ThreadPool.h>
#include <trc_util/functional/Maybe.h>

#include "trc/Types.h"
#include "trc/core/RenderStage.h"

namespace trc
{
    class Window;
    class RenderPass;
    class RenderGraph;
    class RenderConfig;
    class SceneBase;
    class FrameRenderState;

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

        auto record(RenderConfig& config, const SceneBase& scene, FrameRenderState& state) const
            -> std::vector<vk::CommandBuffer>;

        void addPass(RenderStage::ID stage, RenderPass& newPass);
        void removePass(RenderStage::ID stage, RenderPass& pass);

    private:
        struct Stage
        {
            RenderStage::ID id;
            std::vector<RenderPass*> renderPasses;

            std::vector<vk::Event> signalEvents;
            std::vector<vk::Event> waitEvents;
        };

        /**
         * @return Maybe<vk::CommandBuffer> Nothing if no commands were
         *         recorded to the command buffer.
         */
        auto recordStage(vk::CommandBuffer cmdBuf,
                         RenderConfig& config,
                         const SceneBase& scene,
                         FrameRenderState& frameState,
                         const Stage& stage) const
            -> functional::Maybe<vk::CommandBuffer>;

        auto getStage(RenderStage::ID stage) -> Stage*;

        std::vector<Stage> stages;

        std::vector<vk::UniqueEvent> events;
        std::vector<vk::UniqueCommandPool> commandPools;
        std::vector<FrameSpecific<vk::UniqueCommandBuffer>> commandBuffers;

        u_ptr<async::ThreadPool> threadPool{ new async::ThreadPool };
    };
} // namespace trc
