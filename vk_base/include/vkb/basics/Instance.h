#pragma once

#include "VulkanInclude.h"

#include "VulkanDebug.h"

namespace vkb
{

/*
A proxy class for the vk::Instance that initializes the instance
on construction time.
*/
class VulkanInstance
{
public:
    VulkanInstance();

    auto inline operator->() const noexcept -> const vk::Instance* {
        return &*instance;
    }

    auto inline operator*() const noexcept -> vk::Instance {
        return *instance;
    }

    auto inline get() const noexcept -> vk::Instance {
        return *instance;
    }

private:
    vk::UniqueInstance instance;
    std::unique_ptr<VulkanDebug> debug;
};

} // namespace vkb
