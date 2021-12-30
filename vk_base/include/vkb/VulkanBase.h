#pragma once

#include "Instance.h"
#include "VulkanDebug.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "Swapchain.h"

#include "Memory.h"
#include "MemoryPool.h"
#include "Buffer.h"
#include "Image.h"

#include "event/Event.h"

namespace vkb
{
    struct VulkanBaseInitInfo
    {
        bool startEventThread{ true };
    };

    void init(const VulkanBaseInitInfo& info = {});
    void terminate();
} // namespace vkb
