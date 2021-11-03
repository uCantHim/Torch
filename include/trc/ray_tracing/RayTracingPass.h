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
        RayTracingPass() : RenderPass({}, 1) {}

        void begin(vk::CommandBuffer, vk::SubpassContents) override {}
        void end(vk::CommandBuffer) override {}
    };
} // namespace trc
