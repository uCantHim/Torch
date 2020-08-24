#pragma once

#include "../basics/Swapchain.h"

namespace vkb
{
    struct SwapchainEvent
    {
        const Swapchain* swapchain;
    };

    struct SwapchainCreateEvent : public SwapchainEvent {};
    struct SwapchainCloseEvent : public SwapchainEvent {};
    struct SwapchainDestroyEvent : public SwapchainEvent {};
    struct SwapchainResizeEvent : public SwapchainEvent {};

    using WindowCreateEvent = SwapchainCreateEvent;
    using WindowCloseEvent = SwapchainCloseEvent;
    using WindowDestroyEvent = SwapchainDestroyEvent;
    using WindowResizeEvent = SwapchainResizeEvent;
} // namespace vkb
