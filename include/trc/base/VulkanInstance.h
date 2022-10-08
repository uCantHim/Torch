#pragma once

#include "trc/VulkanInclude.h"

#include "trc/base/VulkanDebug.h"

namespace trc
{

/**
 * A proxy class for the vk::Instance that initializes the instance
 * on construction time.
 */
class VulkanInstance
{
public:
    VulkanInstance();

    /**
     * @brief Constructor for non-default initialization values
     *
     * Hint: Create a version with the VK_MAKE_VERSION macro.
     */
    VulkanInstance(const std::string& appName, uint32_t appVersion,
                   const std::string& engineName, uint32_t engineVersion,
                   uint32_t vulkanApiVersion,
                   std::vector<const char*> instanceExtensions);

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

} // namespace trc
