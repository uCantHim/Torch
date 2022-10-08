#pragma once

#include "trc/base/Swapchain.h"

namespace trc
{
    struct SwapchainEvent
    {
        const Swapchain* swapchain;
    };

    struct SwapchainCreateEvent : public SwapchainEvent {};
    using SwapchainRecreateEvent = SwapchainCreateEvent;
    using SwapchainResizeEvent = SwapchainCreateEvent;

    struct PreSwapchainRecreateEvent : public SwapchainEvent {};

    /**
     * Dispatched when the OS requests the window to close.
     */
    struct SwapchainCloseEvent : public SwapchainEvent {};
} // namespace trc
