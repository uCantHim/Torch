#include "VulkanInstance.h"

#include <iostream>

#include <GLFW/glfw3.h>



std::vector<const char*> getRequiredInstanceExtensions()
{
    uint32_t requiredExtensionCount = 0;
    auto requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    std::vector<const char*> extensions(requiredExtensions, requiredExtensions + requiredExtensionCount);

    if constexpr (vkb::VKB_DEBUG) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
        VK_API_VERSION_1_2
    );

    vk::ValidationFeatureEnableEXT enables[] {
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
    };

    vk::StructureChain chain{
        vk::InstanceCreateInfo({}, &appInfo, layers, extensions),
#ifdef TRC_DEBUG
        vk::ValidationFeaturesEXT(1, enables),
#endif
    };

    instance = vk::createInstanceUnique(chain.get());

    if constexpr (enableVerboseLogging)
    {
        std::cout << "Vulkan instance created successfully.\n";

        std::cout << "   Enabled validation layers:\n";
        for (const auto& name : layers) {
            std::cout << "    - " << name << "\n";
        }

        std::cout << "   Enabled instance extensions:\n";
        for (const auto& name : extensions) {
            std::cout << "    - " << name << "\n";
        }
    }

    debug = std::make_unique<VulkanDebug>(*instance);
}
