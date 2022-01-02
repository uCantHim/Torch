#pragma once

#include "core/RenderPass.h"

namespace trc
{
    /**
     * @brief A renderpass that does nothing
     */
    class RayTracingPass : public trc::RenderPass
    {
    public:
        RayTracingPass() : RenderPass({}, 0) {}

        void begin(vk::CommandBuffer, vk::SubpassContents) override;
        void end(vk::CommandBuffer) override {}

        /**
         * @brief Add a function to be executed
         */
        void addRayFunction(std::function<void(vk::CommandBuffer)> func);

    private:
        std::vector<std::function<void(vk::CommandBuffer)>> rayFunctions;
    };
} // namespace trc
