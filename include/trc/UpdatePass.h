#pragma once

#include <functional>

#include "core/RenderStage.h"
#include "core/RenderPass.h"

namespace trc
{
    inline RenderStage resourceUpdateStage{};

    /**
     * @brief A render pass used for resource updates
     */
    class UpdatePass : public RenderPass
    {
    public:
        UpdatePass() : RenderPass({}, 1) {}

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents) final
        {
            update(cmdBuf);
        }

        void end(vk::CommandBuffer) final { /* empty */ }

        virtual void update(vk::CommandBuffer cmdBuf) = 0;
    };

    /**
     * @brief Update pass wrapper that calls a single function
     */
    class UpdateFunctionPass : public UpdatePass
    {
    public:
        explicit UpdateFunctionPass(std::function<void(vk::CommandBuffer)> func)
            : updateFunction(std::move(func))
        {}

        void update(vk::CommandBuffer cmdBuf) final
        {
            updateFunction(cmdBuf);
        }

    private:
        std::function<void(vk::CommandBuffer)> updateFunction;
    };
} // namespace trc
