#include "VulkanDebug.h"

#include <optional>
#include <iostream>



auto vkb::getRequiredValidationLayers() -> std::vector<const char*>
{
    if constexpr (!VKB_DEBUG) {
        return {};
    }

    static const std::vector<const char*> enabledValidationLayers = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_monitor",
        "VK_LAYER_MESA_overlay",

        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_standard_validation",
        "VK_LAYER_LUNARG_image",
        "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_GOOGLE_unique_objects",

        "VK_LAYER_NV_optimus",
    };
    std::vector<const char*> result;

    auto availableLayers = vk::enumerateInstanceLayerProperties();
    for (const auto& enabledLayer : enabledValidationLayers)
    {
        for (const auto& availableLayer : availableLayers) {
            if (strcmp(enabledLayer, static_cast<const char*>(availableLayer.layerName)) == 0) {
                result.push_back(enabledLayer);
                break;
            }
        }
    }

    return result;
}



vkb::VulkanDebug::VulkanDebug(vk::Instance instance)
    :
    instance(instance),
    dispatcher(instance, vkGetInstanceProcAddr)
{
    if constexpr (!VKB_DEBUG) {
        return;
    }

    vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        {}, // Create flags
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
        ,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        ,
        vulkanDebugCallbackWrapper,
        nullptr
    );

    debugMessenger = instance.createDebugUtilsMessengerEXTUnique(createInfo, nullptr, dispatcher);
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkb::VulkanDebug::vulkanDebugCallbackWrapper(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    vulkanDebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity),
        vk::DebugUtilsMessageTypeFlagBitsEXT(messageType),
        vk::DebugUtilsMessengerCallbackDataEXT(*callbackData),
        userData
    );

    return VK_FALSE;
}

void vkb::VulkanDebug::vulkanDebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT& callbackData,
    void*)
{
    std::stringstream ss;
    switch (messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        std::cerr << "A " << vk::to_string(messageType) << " error occured."
            << " Check the error log for additional details.\n";
        std::cerr << callbackData.pMessage << "\n";

        ss << "A " + vk::to_string(messageType) + " error occured:\n";
        ss << callbackData.pMessage << "\n";
        ss << "Involved objects:\n";
        for (uint32_t i = 0; i < callbackData.objectCount; i++)
        {
            auto& obj = callbackData.pObjects[i];
            ss << " - " << obj.objectHandle << "(" << vk::to_string(obj.objectType) << "): "
                << (obj.pObjectName != nullptr ? obj.pObjectName : "")
                << "\n";
        }

        vkErrorLog << ss.rdbuf();
        throw std::runtime_error(ss.str());
        break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        std::cout << "A warning occured: " << callbackData.pMessage << "\n";
        vkWarningLog << "A " + vk::to_string(messageType) + " warning occured:\n";
        vkWarningLog << callbackData.pMessage << "\n";
        break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        vkInfoLog << callbackData.pMessage << "\n";
        break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        vkVerboseLog << callbackData.pMessage << "\n";
        break;

    default:
        throw std::logic_error("Message severity was unknown enum.");
    }
}
