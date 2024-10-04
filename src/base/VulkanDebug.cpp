#include "trc/base/VulkanDebug.h"

#include <cassert>
#include <cstdlib>

#include <stdexcept>



auto trc::getRequiredValidationLayers() -> std::vector<const char*>
{
#ifndef TRC_DEBUG
    return {};
#else
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
#endif
}



trc::VulkanDebug::VulkanDebug(vk::Instance instance)
#ifdef TRC_DEBUG
    :
    instance(instance),
    dispatcher(instance, vkGetInstanceProcAddr),
    debugLogger(std::make_unique<VulkanDebugLogger>())
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        {}, // Create flags
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
        ,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        ,
        vulkanDebugCallbackWrapper,
        debugLogger.get()
    );

    debugMessenger = instance.createDebugUtilsMessengerEXTUnique(createInfo, nullptr, dispatcher);
}
#else
{
}
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL trc::VulkanDebug::vulkanDebugCallbackWrapper(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    assert(userData != nullptr);
    static_cast<VulkanDebugLogger*>(userData)->vulkanDebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity),
        vk::DebugUtilsMessageTypeFlagBitsEXT(messageType),
        vk::DebugUtilsMessengerCallbackDataEXT(*callbackData)
    );

    // "The application *should* always return VK_FALSE. The VK_TRUE value is
    // reserved for use in layer development."
    return VK_FALSE;
}



trc::VulkanDebug::VulkanDebugLogger::VulkanDebugLogger()
    :
#ifdef TRC_DEBUG_THROW_ON_VALIDATION_ERROR
    throwOnValidationError(true)
#else
    throwOnValidationError(std::getenv("TORCH_DEBUG_THROW_ON_VALIDATION_ERROR") != nullptr)
#endif
{
    if (auto str = std::getenv("TORCH_DEBUG_LOG_DIR"); str != nullptr) {
        debugLogDir = str;
    }

    vkErrorLogFile   = { debugLogDir / "vulkan_error.log" };
    vkWarningLogFile = { debugLogDir / "vulkan_warning.log" };
    vkInfoLogFile    = { debugLogDir / "vulkan_info.log" };
    vkVerboseLogFile = { debugLogDir / "vulkan_verbose.log" };

    vkErrorLog   = Logger<log::LogLevel::eError>{ vkErrorLogFile, log::makeDefaultLogHeader("ERROR") };
    vkWarningLog = Logger<log::LogLevel::eWarning>{ vkWarningLogFile, log::makeDefaultLogHeader("WARNING") };
    vkInfoLog    = Logger<log::LogLevel::eInfo>{ vkInfoLogFile, log::makeDefaultLogHeader("INFO") };
    vkVerboseLog = Logger<log::LogLevel::eDebug>{ vkVerboseLogFile, log::makeDefaultLogHeader("VERBOSE") };

    if (!fs::is_directory(debugLogDir)) {
        fs::create_directories(debugLogDir);
    }
}

void trc::VulkanDebug::VulkanDebugLogger::vulkanDebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT& callbackData)
{
    switch (messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        log::error << "A Vulkan " << vk::to_string(messageType) << " error occured: "
                   << callbackData.pMessage;

        vkErrorLog << "A Vulkan " + vk::to_string(messageType) + " error occured:";
        vkErrorLog << callbackData.pMessage;
        vkErrorLog << "Involved objects:";
        for (uint32_t i = 0; i < callbackData.objectCount; i++)
        {
            auto& obj = callbackData.pObjects[i];
            vkErrorLog << " - " << obj.objectHandle << " (" << vk::to_string(obj.objectType) << ")"
                       << " [" << (obj.pObjectName != nullptr ? obj.pObjectName : "") << "]";
        }

        if (throwOnValidationError) {
            throw std::runtime_error("A Vulkan " + vk::to_string(messageType) + " error occured.");
        }
        break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        log::warn << "A Vulkan " << vk::to_string(messageType) << " warning occured: "
                  << callbackData.pMessage;

        vkWarningLog << "A " + vk::to_string(messageType) + " warning occured:";
        vkWarningLog << callbackData.pMessage;
        vkWarningLog << "Involved objects:";
        for (uint32_t i = 0; i < callbackData.objectCount; i++)
        {
            auto& obj = callbackData.pObjects[i];
            vkWarningLog << " - " << obj.objectHandle
                         << " (" << vk::to_string(obj.objectType) << ")"
                         << " [" << (obj.pObjectName != nullptr ? obj.pObjectName : "") << "]";
        }
        break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        vkInfoLog << callbackData.pMessage;
        break;

    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        vkVerboseLog << callbackData.pMessage;
        break;

    default:
        throw std::logic_error("Message severity was unknown enum.");
    }
}
