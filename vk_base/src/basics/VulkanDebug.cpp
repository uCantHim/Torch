#include "VulkanDebug.h"

#include <iostream>

#include "VulkanEXT.h"



vkb::VulkanDebug::VulkanDebug(vk::Instance instance)
    :
    instance(instance)
{
    if constexpr (!debugMode) {
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
        vulkanDebugCallback,
        nullptr
    );

    VkDebugUtilsMessengerCreateInfoEXT _createInfo_cast = createInfo;
    if (vkEXT::CreateDebugUtilsMessengerEXT(
        instance,
        &_createInfo_cast,
        nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Unable to initialize the Vulkan debugger.");
    }
}


vkb::VulkanDebug::~VulkanDebug() noexcept
{
    vkEXT::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}


VKAPI_ATTR VkBool32 VKAPI_CALL vkb::VulkanDebug::vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*)
{
    std::string errorTypeStr;
    auto objects = callbackData->pObjects;

    switch (messageType)
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        errorTypeStr = "general";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        errorTypeStr = "performance";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        errorTypeStr = "validation";
        break;
    default:
        throw std::logic_error("Message type was unknown enum.");
    }


    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        std::cerr << "A " << errorTypeStr << " error occured. Check the error log for additional details.\n";
        std::cerr << callbackData->pMessage << "\n";

        vkErrorLog.log("A " + errorTypeStr + " error occured:");
        vkErrorLog.log(callbackData->pMessage);
        vkErrorLog.log("Involved objects:");

        for (uint32_t i = 0; i < callbackData->objectCount; i++)
        {
            auto& obj = objects[i];
            vkErrorLog.log(
                " - " + std::to_string(obj.objectHandle)
                + "(" + std::to_string(obj.objectType) + "): "
                + (obj.pObjectName != nullptr ? obj.pObjectName : "")
            );
        }
        throw std::runtime_error("A vulkan error occured.\n");
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        std::cout << "A warning occured: " << callbackData->pMessage << "\n";
        vkWarningLog.log("A " + errorTypeStr + " warning occured:");
        vkWarningLog.log(callbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        vkInfoLog.log(callbackData->pMessage);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        vkVerboseLog.log(callbackData->pMessage);
        break;

    default:
        throw std::logic_error("Message severity was unknown enum.");
    }

    return VK_FALSE;
}


auto vkb::getRequiredValidationLayers() -> std::vector<const char*>
{
    if constexpr (!debugMode) {
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
        "VK_LAYER_MESA_device_select",
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

    if constexpr(enableVerboseLogging) {
        std::cout << "\nEnabled validation layers:\n";
        for (const auto& name : result) {
            std::cout << " - " << name << "\n";
        }
    }

    return result;
}
