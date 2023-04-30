#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "trc/VulkanInclude.h"
#include "VulkanDebug.h"

namespace trc
{

struct VulkanInstanceCreateInfo
{
    const std::string& appName{ "Your application" };
    uint32_t appVersion{ VK_MAKE_VERSION(0, 0, 1) };

    const std::string& engineName{ "Torch" };
    uint32_t engineVersion{ VK_MAKE_VERSION(0, 0, 1) };

    uint32_t vulkanApiVersion{ VK_API_VERSION_1_3 };

    // A list of instance extension names that shall be enabled on the instance.
    //
    // If `TORCH_DEBUG` is defined, the instance always enables `VK_EXT_debug_utils`
    // in addition to the contents of this member.
    // Additionally, if surface creation is at all supported, all required
    // extensions for creation of Vulkan surfaces will be enabled as well.
    std::vector<const char*> instanceExtensions{};

    // `vk::ValidationFeaturesEXT` will only be enabled if `TORCH_DEBUG` is defined.
    std::vector<vk::ValidationFeatureEnableEXT> enabledValidationFeatures{
        vk::ValidationFeatureEnableEXT::eBestPractices,
        vk::ValidationFeatureEnableEXT::eDebugPrintf,
    };

    // `vk::ValidationFeaturesEXT` will only be enabled if `TORCH_DEBUG` is defined.
    std::vector<vk::ValidationFeatureDisableEXT> disabledValidationFeatures{};
};

/**
 * A proxy class for the vk::Instance that initializes the instance
 * on construction time.
 */
class VulkanInstance
{
public:
    /**
     * @brief Construct a Vulkan instance
     */
    explicit VulkanInstance(const VulkanInstanceCreateInfo& createInfo = {});

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
