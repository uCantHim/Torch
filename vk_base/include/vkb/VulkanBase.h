#pragma once

#include "basics/Instance.h"
#include "basics/VulkanDebug.h"
#include "basics/PhysicalDevice.h"
#include "basics/Device.h"
#include "basics/Swapchain.h"

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
