#include "Instance.h"

#include <iostream>

#include <GLFW/glfw3.h>



std::vector<const char*> getRequiredInstanceExtensions()
{
    uint32_t requiredExtensionCount = 0;
    auto requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    std::vector<const char*> extensions(requiredExtensions, requiredExtensions + requiredExtensionCount);

    if constexpr (vkb::debugMode) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "\nRequired instance extensions:\n";
        for (const auto& name : extensions) {
            std::cout << " - " << name << "\n";
        }
    }

    return extensions;
}


vkb::VulkanInstance::VulkanInstance()
{
    auto layers = getRequiredValidationLayers();
    auto extensions = getRequiredInstanceExtensions();

    vk::ApplicationInfo appInfo(
        "Improved Vulkan application", VK_MAKE_VERSION(1, 2, 0),
        "No engine", VK_MAKE_VERSION(1, 2, 0),
        VK_API_VERSION_1_2);

    vk::InstanceCreateInfo createInfo(
        {}, &appInfo,
        static_cast<uint32_t>(layers.size()), layers.data(),
        static_cast<uint32_t>(extensions.size()), extensions.data());

    instance = vk::createInstanceUnique(createInfo);
    if constexpr (enableVerboseLogging) {
        std::cout << "Instance created successfully.\n";
    }

    debug = std::make_unique<VulkanDebug>(*instance);
}
