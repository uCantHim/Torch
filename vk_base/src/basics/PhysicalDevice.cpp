#include "PhysicalDevice.h"

#include <set>
#include <iostream>

#include "VulkanDebug.h"



// ------------------------------------ //
//        Physical device helpers       //
// ------------------------------------ //

auto vkb::phys_device_properties::findSupportedQueueFamilies(
    const vk::PhysicalDevice& device,
    const vk::SurfaceKHR& surface) -> QueueFamilies
{
    QueueFamilies result;
    auto queueFamilyProperties = device.getQueueFamilyProperties();

    for (size_t familyIndex = 0; familyIndex < queueFamilyProperties.size(); familyIndex++)
    {
        const auto& familyProperty = queueFamilyProperties[familyIndex];
        if (familyProperty.queueCount <= 0) {
            continue;
        }

        auto queueCapabilities = familyProperty.queueFlags;
        bool presentCapability = device.getSurfaceSupportKHR(
            static_cast<uint32_t>(familyIndex), surface
        );

        QueueFamily newFamily = {
            static_cast<uint32_t>(familyIndex),
            familyProperty.queueCount,
            {
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eGraphics),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eCompute),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eTransfer),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eSparseBinding),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eProtected),
                presentCapability

            }
        };

        if (newFamily.isCapable(queue_type::graphics)) {
            result.graphicsFamilies.push_back(newFamily);
        }
        if (newFamily.isCapable(queue_type::compute)) {
            result.computeFamilies.push_back(newFamily);
        }
        if (newFamily.isCapable(queue_type::transfer)) {
            result.transferFamilies.push_back(newFamily);
        }
        if (newFamily.isCapable(queue_type::sparseMemory)) {
            result.sparseMemoryFamilies.push_back(newFamily);
        }
        if (newFamily.isCapable(queue_type::protectedMemory)) {
            result.protectedMemoryFamilies.push_back(newFamily);
        }
        if (newFamily.isCapable(queue_type::presentation)) {
            result.presentationFamilies.push_back(newFamily);
        }
    }

    return result;
}


auto vkb::phys_device_properties::findSwapchainSupport(
    const vk::PhysicalDevice& device,
    const vk::SurfaceKHR& surface) -> SwapchainSupport
{
    SwapchainSupport result;

    result.surfaceCapabilities = device.getSurfaceCapabilitiesKHR(surface);
    result.surfaceFormats = device.getSurfaceFormatsKHR(surface);
    result.surfacePresentModes = device.getSurfacePresentModesKHR(surface);

    return result;
}


/**
 * This must be a function (instead of a global constant) because the
 * device is initialized at static time. The global constant would be
 * initialized at a later time. Thus, this vector would be empty at
 * device initialization time, which would cause Vulkan to crash.
 */
static auto getRequiredDeviceExtensions() -> std::vector<const char*> {
    return std::vector<const char*> {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}



// ---------------------------- //
//        Physcial device       //
// ---------------------------- //

using namespace vkb::phys_device_properties;

vkb::PhysicalDevice::PhysicalDevice(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    :
    physicalDevice(device),
    queueFamilies(findSupportedQueueFamilies(device, surface)),
    supportedExtensions(device.enumerateDeviceExtensionProperties()),
    properties(device.getProperties()),
    features(device.getFeatures()),
    memoryProperties(device.getMemoryProperties())
{
    // Logging
    if constexpr (enableVerboseLogging) {
        switch (properties.deviceType)
        {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            typeString = "discrete GPU"; break;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            typeString = "integrated GPU"; break;
        case vk::PhysicalDeviceType::eVirtualGpu:
            typeString = "virtual GPU"; break;
        case vk::PhysicalDeviceType::eCpu:
            typeString = "CPU"; break;
        case vk::PhysicalDeviceType::eOther:
            typeString = "unkown type"; break;
        }
        name = std::string(properties.deviceName);
        std::cout << "\nFound device \"" << name << "\" (" << typeString << "):\n";

        // Print queue family info
        auto uniqueQueueFamilies = getUniqueQueueFamilies();
        std::cout << uniqueQueueFamilies.size() << " queue families:\n";
        for (const auto& fam : uniqueQueueFamilies) {
            std::cout << " - Queue family #" << fam.index << "\n";
            std::cout << "\t" << fam.queueCount << " queues\n";
            if (fam.isCapable(queue_type::graphics))
                std::cout << "\tgraphics capable\n";
            if (fam.isCapable(queue_type::compute))
                std::cout << "\tcompute capable\n";
            if (fam.isCapable(queue_type::transfer))
                std::cout << "\ttransfer capable\n";
            if (fam.isCapable(queue_type::sparseMemory))
                std::cout << "\tsparse memory capable\n";
            if (fam.isCapable(queue_type::protectedMemory))
                std::cout << "\tprotected memory capable\n";
            if (fam.isCapable(queue_type::presentation))
                std::cout << "\tpresentation capable\n";
        }
    }
}


auto vkb::PhysicalDevice::createLogicalDevice() const -> vk::UniqueDevice
{
    // Device queues
    auto uniqueQueueFamilies = getUniqueQueueFamilies();
    std::vector<float> prios(100, 1.0f); // Enough prios for 100 queues per family
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const auto& queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.push_back(
            { {}, queueFamily.index, queueFamily.queueCount, prios.data() }
        );
    }

    // Device features
    const vk::PhysicalDeviceFeatures deviceFeatures = {};

    // Validation validationLayers
    const auto validationLayers = getRequiredValidationLayers();

    // Extensions
    const auto deviceExtensions = getRequiredDeviceExtensions();

    // Create the logical device
    vk::DeviceCreateInfo createInfo(
        {},
        static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
        static_cast<uint32_t>(validationLayers.size()), validationLayers.data(),
        static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(),
        &deviceFeatures
    );

    return physicalDevice.createDeviceUnique(createInfo);
}


auto vkb::PhysicalDevice::getUniqueQueueFamilies() const noexcept
    -> std::vector<QueueFamily>
{
    std::vector<QueueFamily> result;
    std::set<uint32_t> gatheredFamilies;

    for (const auto& family : queueFamilies.graphicsFamilies) {
        const auto [it, success] = gatheredFamilies.insert(family.index);
        if (success) {
            result.push_back(family);
        }
    }
    for (const auto& family : queueFamilies.computeFamilies) {
        const auto [it, success] = gatheredFamilies.insert(family.index);
        if (success) {
            result.push_back(family);
        }
    }
    for (const auto& family : queueFamilies.transferFamilies) {
        const auto [it, success] = gatheredFamilies.insert(family.index);
        if (success) {
            result.push_back(family);
        }
    }
    for (const auto& family : queueFamilies.sparseMemoryFamilies) {
        const auto [it, success] = gatheredFamilies.insert(family.index);
        if (success) {
            result.push_back(family);
        }
    }
    for (const auto& family : queueFamilies.protectedMemoryFamilies) {
        const auto [it, success] = gatheredFamilies.insert(family.index);
        if (success) {
            result.push_back(family);
        }
    }
    for (const auto& family : queueFamilies.presentationFamilies) {
        const auto [it, success] = gatheredFamilies.insert(family.index);
        if (success) {
            result.push_back(family);
        }
    }

    return result;
}


auto vkb::PhysicalDevice::getSwapchainSupport(const vk::SurfaceKHR& surface) const noexcept
    -> phys_device_properties::SwapchainSupport
{
    return findSwapchainSupport(physicalDevice, surface);
}


uint32_t vkb::PhysicalDevice::findMemoryType(
    uint32_t requiredMemoryTypeBits,
    vk::MemoryPropertyFlags requiredProperties) const
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if (requiredMemoryTypeBits & (1 << i) &&
            (memoryProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties)
        {
            return i;
        }
    }

    throw std::runtime_error("Unable to find appropriate memory type.");
}



// ---------------------------- //
//        Helper functions      //
// ---------------------------- //

auto vkb::device_helpers::findAllPhysicalDevices(vk::Instance instance, vk::SurfaceKHR surface)
    -> std::vector<PhysicalDevice>
{
    auto availableDevices = instance.enumeratePhysicalDevices();
    if (availableDevices.empty()) {
        throw std::runtime_error("Unable to find any physical graphics devices!"
                                 " You may want to visit the Nvidia store :)");
    }

    // Create all available physical devices
    std::vector<PhysicalDevice> detectedDevices;
    for (const auto& device : availableDevices) {
        detectedDevices.push_back(PhysicalDevice(device, surface));
    }

    return detectedDevices;
}

auto vkb::device_helpers::getOptimalPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
    -> PhysicalDevice
{
    auto detectedDevices = findAllPhysicalDevices(instance, surface);

    // Decide on optimal device.
    // The decision algorithm is not complex at all right now, but could be improved easily.
    for (const auto& device : detectedDevices)
    {
        if (isOptimalDevice(device)) {
            if constexpr (enableVerboseLogging) {
                std::cout << "Found optimal physical device: \"" << device.name << "\"!\n";
            }
            return device;
        }
        if constexpr (enableVerboseLogging) {
            std::cout << device.name << " is a suboptimal physical device.\n";
        }
    }

    throw std::runtime_error("Unable to find a physical device that meets the criteria.");
}


bool vkb::device_helpers::isOptimalDevice(const PhysicalDevice& device)
{
    return supportsRequiredDeviceExtensions(device)
        && supportsRequiredQueueFamilies(device);
}


bool vkb::device_helpers::supportsRequiredQueueFamilies(const PhysicalDevice& device)
{
    using namespace phys_device_properties;
    const auto& families = device.queueFamilies;

    return !families.graphicsFamilies.empty()
        && !families.presentationFamilies.empty()
        && !families.transferFamilies.empty();
}


bool vkb::device_helpers::supportsRequiredDeviceExtensions(const PhysicalDevice& device)
{
    const auto requiredDeviceExtensions = getRequiredDeviceExtensions();
    std::set<std::string> requiredExtensions(
        requiredDeviceExtensions.begin(),
        requiredDeviceExtensions.end());

    for (const auto& supportedExt : device.supportedExtensions) {
        requiredExtensions.erase(static_cast<const char*>(supportedExt.extensionName));
    }

    return requiredExtensions.empty();
}
