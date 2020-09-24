#pragma once

#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

#include <vulkan/vulkan.hpp>

#include "../Logger.h"

namespace vkb
{

#ifdef MY_DEBUG
constexpr auto debugMode = true;
#else
constexpr auto debugMode = false;
#endif

/**
 * Various vulkan-related methods print a lot of information
 * if this is enabled.
 */
constexpr bool enableVerboseLogging = debugMode;

extern std::vector<const char*> getRequiredValidationLayers();

class VulkanDebug
{
public:
    explicit VulkanDebug(vk::Instance instance);
    VulkanDebug(const VulkanDebug&) = delete;
    VulkanDebug(VulkanDebug&&) = delete;
    ~VulkanDebug() noexcept;

    VulkanDebug& operator=(const VulkanDebug&) = delete;
    VulkanDebug& operator=(VulkanDebug&&) = delete;

private:
    const vk::Instance instance; // Required for messenger destruction
    VkDebugUtilsMessengerEXT debugMessenger{};

    static inline const bool _init = []() -> bool {
        if (!fs::is_directory("vulkan_logs")) {
            fs::create_directory("vulkan_logs");
        }
        return true;
    }();
    static inline Logger vkErrorLog   { "vulkan_logs/vulkan_error.log" };
    static inline Logger vkWarningLog { "vulkan_logs/vulkan_warning.log" };
    static inline Logger vkInfoLog    { "vulkan_logs/vulkan_info.log" };
    static inline Logger vkVerboseLog { "vulkan_logs/vulkan_verbose.log" };

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData
    );
};

} // namespace vkb
