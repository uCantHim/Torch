#include "trc/base/VulkanInstance.h"

#include <GLFW/glfw3.h>

#include "trc/base/VulkanDebug.h"



std::vector<const char*> getRequiredInstanceExtensions()
{
    uint32_t requiredExtensionCount = 0;
    auto requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    std::vector<const char*> extensions(requiredExtensions, requiredExtensions + requiredExtensionCount);

    if constexpr (trc::TRC_DEBUG_BUILD) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}


trc::VulkanInstance::VulkanInstance()
    :
    VulkanInstance(
        "No application", VK_MAKE_VERSION(0, 0, 0),
        "No engine", VK_MAKE_VERSION(0, 0, 0),
        VK_API_VERSION_1_3,
        {}
    )
{
}

trc::VulkanInstance::VulkanInstance(
    const std::string& appName,
    uint32_t appVersion,
    const std::string& engineName,
    uint32_t engineVersion,
    uint32_t vulkanApiVersion,
    std::vector<const char*> instanceExtensions)
{
    auto layers = getRequiredValidationLayers();
    auto extensions = getRequiredInstanceExtensions();
    extensions.insert(extensions.end(), instanceExtensions.begin(), instanceExtensions.end());

    vk::ApplicationInfo appInfo(
        appName.c_str(), appVersion,
        engineName.c_str(), engineVersion,
        vulkanApiVersion
    );

#ifdef TRC_DEBUG
    vk::ValidationFeatureEnableEXT enables[] {
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
    };
#endif

    vk::StructureChain chain{
        vk::InstanceCreateInfo({}, &appInfo, layers, extensions),
#ifdef TRC_DEBUG
        vk::ValidationFeaturesEXT(1, enables),
#endif
    };

    instance = vk::createInstanceUnique(chain.get());

    log::info << "Vulkan instance created successfully.\n";
    log::info << "   Enabled validation layers:\n";
    for (const auto& name : layers) {
        log::info << "    - " << name << "\n";
    }
    log::info << "   Enabled instance extensions:\n";
    for (const auto& name : extensions) {
        log::info << "    - " << name << "\n";
    }

    debug = std::make_unique<VulkanDebug>(*instance);
}
