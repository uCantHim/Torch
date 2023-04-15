#include "trc/base/VulkanDebug.h"

#include <optional>



auto trc::getRequiredValidationLayers() -> std::vector<const char*>
{
    if constexpr (!TRC_DEBUG_BUILD) {
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



trc::VulkanDebug::VulkanDebug(vk::Instance instance)
    :
    instance(instance),
    dispatcher(instance, vkGetInstanceProcAddr)
{
    if constexpr (!TRC_DEBUG_BUILD) {
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

VKAPI_ATTR VkBool32 VKAPI_CALL trc::VulkanDebug::vulkanDebugCallbackWrapper(
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

void trc::VulkanDebug::vulkanDebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT& callbackData,
    void*)
{
    std::stringstream ss;
    switch (messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        log::error << "A Vulkan " << vk::to_string(messageType) << " error occured."
            << " Check the error log for additional details.\n";
        log::error << callbackData.pMessage;

        ss << "A Vulkan " + vk::to_string(messageType) + " error occured:\n";
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
        throw std::runtime_error("A Vulkan " + vk::to_string(messageType) + " error occured");
        break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        log::warn << "A warning occured: " << callbackData.pMessage;
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
