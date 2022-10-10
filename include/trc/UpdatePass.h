#pragma once

#include <functional>

#include "trc/core/RenderPass.h"

namespace trc
{
    /**
     * @brief A render pass used for resource updates
     */
    class UpdatePass : public RenderPass
    {
    public:
        UpdatePass() : RenderPass({}, 1) {}

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents, FrameRenderState& state) final
        {
            update(cmdBuf, state);
        }

        void end(vk::CommandBuffer) final { /* empty */ }

        virtual void update(vk::CommandBuffer cmdBuf, FrameRenderState& frameState) = 0;
    };

    /**
     * @brief Update pass wrapper that calls a single function
     */
    class UpdateFunctionPass : public UpdatePass
    {
    public:
        explicit UpdateFunctionPass(std::function<void(vk::CommandBuffer, FrameRenderState&)> func)
            : updateFunction(std::move(func))
        {}

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) final
        {
            updateFunction(cmdBuf, state);
        }

    private:
        std::function<void(vk::CommandBuffer, FrameRenderState&)> updateFunction;
    };
} // namespace trc
