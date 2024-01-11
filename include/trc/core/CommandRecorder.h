#pragma once

#include <optional>
#include <vector>

#include <trc_util/async/ThreadPool.h>

#include "trc/Types.h"
#include "trc/base/Device.h"
#include "trc/base/FrameSpecificObject.h"
#include "trc/core/DrawConfiguration.h"
#include "trc/core/RenderStage.h"
#include "trc/core/RenderGraph.h"

namespace trc
{
    class Window;
    class RenderPass;
    class RenderGraph;
    class RenderConfig;
    class RasterSceneBase;
    class FrameRenderState;

    /**
     * @brief A command collection policy
     *
     * Defines the structure of a complete image synthesis pathway. Can
     * declare stages in which one or more render passes can be executed.
     */
    class CommandRecorder
    {
    public:
        CommandRecorder(const CommandRecorder&) = delete;
        auto operator=(const CommandRecorder&) -> CommandRecorder& = delete;

        explicit CommandRecorder(const Device& device, const FrameClock& frameClock);
        CommandRecorder(CommandRecorder&&) noexcept = default;
        ~CommandRecorder() = default;

        auto operator=(CommandRecorder&&) noexcept -> CommandRecorder& = default;

        auto record(const vk::ArrayProxy<const DrawConfig>& draws, FrameRenderState& state)
            -> std::vector<vk::CommandBuffer>;

    private:
        struct PerFrame
        {
            // "Use L * T pools. (L = the number of buffered frames,
            //                    T = the number of threads that record command buffers)"
            std::vector<vk::UniqueCommandPool> perThreadPools;
            std::vector<vk::UniqueCommandBuffer> perThreadCmdBuffers;
        };

        /**
         * @return Maybe<vk::CommandBuffer> Nothing if no commands were
         *         recorded to the command buffer.
         */
        auto recordStage(vk::CommandBuffer cmdBuf,
                         RenderConfig& config,
                         const RasterSceneBase& scene,
                         FrameRenderState& frameState,
                         const RenderGraph::StageInfo& stage) const
            -> std::optional<vk::CommandBuffer>;

        const Device* device;

        FrameSpecific<PerFrame> perFrameObjects;
        u_ptr<async::ThreadPool> threadPool{ new async::ThreadPool };
    };
} // namespace trc
