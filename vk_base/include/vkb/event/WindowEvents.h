#pragma once

#include "../basics/Swapchain.h"

namespace vkb
{
    struct SwapchainDestroyEvent
    {
        Swapchain* swapchain;
    };
} // namespace vkb
