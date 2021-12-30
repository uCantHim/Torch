#pragma once

#include <vector>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "VulkanInclude.h"

namespace vkb
{

#ifdef TRC_DEBUG
constexpr auto VKB_DEBUG = true;
#else
constexpr auto VKB_DEBUG = false;
#endif

/**
 * Various vulkan-related methods print a lot of information
 * if this is enabled.
 */
constexpr bool enableVerboseLogging = VKB_DEBUG;

auto getRequiredValidationLayers() -> std::vector<const char*>;

class VulkanDebug
{
public:
    VulkanDebug(const VulkanDebug&) = delete;
    VulkanDebug(VulkanDebug&&) noexcept = delete;
    VulkanDebug& operator=(const VulkanDebug&) = delete;
    VulkanDebug& operator=(VulkanDebug&&) noexcept = delete;

    explicit VulkanDebug(vk::Instance instance);
    ~VulkanDebug() noexcept = default;

private:
    const vk::Instance instance; // Required for messenger destruction
    vk::DispatchLoaderDynamic dispatcher;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debugMessenger;

    static inline const bool _init = []() -> bool {
        if (!fs::is_directory("vulkan_logs")) {
            fs::create_directory("vulkan_logs");
        }
        return true;
    }();

    static inline std::ofstream vkErrorLog  { "vulkan_logs/vulkan_error.log" };
    static inline std::ofstream vkWarningLog{ "vulkan_logs/vulkan_warning.log" };
    static inline std::ofstream vkInfoLog   { "vulkan_logs/vulkan_info.log" };
    static inline std::ofstream vkVerboseLog{ "vulkan_logs/vulkan_verbose.log" };

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallbackWrapper(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData
    );
    static void vulkanDebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT& callbackData,
        void* userData
    );
};

} // namespace vkb
