#include "basics/PhysicalDevice.h"

#include <set>
#include <iostream>



// ------------------------------------ //
//        Physical device helpers       //
// ------------------------------------ //

auto vkb::getQueueFamilies(
    vk::PhysicalDevice device,
    vk::SurfaceKHR surface) -> std::vector<QueueFamily>
{
    std::vector<QueueFamily> result;
    auto queueFamilies = device.getQueueFamilyProperties();

    for (uint32_t familyIndex = 0; const auto& family : queueFamilies)
    {
        const vk::QueueFlags queueCapabilities = family.queueFlags;
        const bool presentCapability = device.getSurfaceSupportKHR(familyIndex, surface);

        result.push_back({
            familyIndex,
            family.queueCount,
            {
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eGraphics),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eCompute),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eTransfer),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eSparseBinding),
                static_cast<bool>(queueCapabilities & vk::QueueFlagBits::eProtected),
                presentCapability
            }
        });
        familyIndex++;
    }

    return result;
}

auto vkb::sortByCapabilities(const std::vector<QueueFamily>& families)
    -> QueueFamilyCapabilities
{
    QueueFamilyCapabilities result;
    for (const auto& family : families)
    {
        if (family.isCapable(QueueType::graphics)) {
            result.graphicsCapable.push_back(family);
        }
        if (family.isCapable(QueueType::compute)) {
            result.computeCapable.push_back(family);
        }
        if (family.isCapable(QueueType::transfer)) {
            result.transferCapable.push_back(family);
        }
        if (family.isCapable(QueueType::sparseMemory)) {
            result.sparseMemoryCapable.push_back(family);
        }
        if (family.isCapable(QueueType::protectedMemory)) {
            result.protectedMemoryCapable.push_back(family);
        }
        if (family.isCapable(QueueType::presentation)) {
            result.presentationCapable.push_back(family);
        }
    }

    return result;
}



// ---------------------------- //
//        Physcial device       //
// ---------------------------- //

vkb::PhysicalDevice::PhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
    :
    PhysicalDevice(device_helpers::getOptimalPhysicalDevice(instance, surface))
{
}

vkb::PhysicalDevice::PhysicalDevice(vk::PhysicalDevice device, vk::SurfaceKHR surface)
    :
    physicalDevice(device),
    queueFamilies(getQueueFamilies(device, surface)),
    queueFamilyCapabilities(sortByCapabilities(queueFamilies)),
    supportedExtensions(device.enumerateDeviceExtensionProperties()),
    properties(device.getProperties()),
    features(device.getFeatures()),
    memoryProperties(device.getMemoryProperties()),
    name(properties.deviceName),
    type(properties.deviceType),
    typeString(vk::to_string(properties.deviceType))
{
    // Logging
    if constexpr (enableVerboseLogging)
    {
        std::cout << "\nFound device \"" << name << "\" (" << typeString << "):\n";

        // Print queue family info
        std::cout << queueFamilies.size() << " queue families:\n";
        for (const auto& fam : queueFamilies)
        {
            std::cout << " - Queue family #" << fam.index << "\n";
            std::cout << "\t" << fam.queueCount << " queues\n";
            if (fam.isCapable(QueueType::graphics))
                std::cout << "\tgraphics capable\n";
            if (fam.isCapable(QueueType::compute))
                std::cout << "\tcompute capable\n";
            if (fam.isCapable(QueueType::transfer))
                std::cout << "\ttransfer capable\n";
            if (fam.isCapable(QueueType::sparseMemory))
                std::cout << "\tsparse memory capable\n";
            if (fam.isCapable(QueueType::protectedMemory))
                std::cout << "\tprotected memory capable\n";
            if (fam.isCapable(QueueType::presentation))
                std::cout << "\tpresentation capable\n";
        }
    }
}

auto vkb::PhysicalDevice::createLogicalDevice(
    std::vector<const char*> deviceExtensions,
    void* extraPhysicalDeviceFeatureChain
    ) const -> vk::UniqueDevice
{
    // Device queues
    const auto maxQueues = [&] {
        uint32_t result = 0;
        for (const auto& f : queueFamilies) {
            result = std::max(result, f.queueCount);
        }
        return result;
    }();
    std::vector<float> prios(maxQueues, 1.0f);
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const auto& queueFamily : queueFamilies)
    {
        queueCreateInfos.push_back(
            { {}, queueFamily.index, queueFamily.queueCount, prios.data() }
        );
    }

    // Validation layers
    const auto validationLayers = getRequiredValidationLayers();

    // Extensions
    const auto requiredDevExt = device_helpers::getRequiredDeviceExtensions();
    deviceExtensions.insert(deviceExtensions.end(), requiredDevExt.begin(), requiredDevExt.end());

    // Default device features
    vk::StructureChain deviceFeatures{
        vk::PhysicalDeviceFeatures2{},
        vk::PhysicalDeviceBufferDeviceAddressFeatures{},
        vk::PhysicalDeviceDescriptorIndexingFeatures{},
    };

    // Add custom features
    deviceFeatures.get<vk::PhysicalDeviceDescriptorIndexingFeatures>()
        .setPNext(extraPhysicalDeviceFeatureChain);

    // Query features
    physicalDevice.getFeatures2(&deviceFeatures.get<vk::PhysicalDeviceFeatures2>());

    // Create the logical device
    vk::StructureChain chain
    {
        vk::DeviceCreateInfo(
            {},
            queueCreateInfos,
            validationLayers,
            deviceExtensions,
            nullptr
        ),
        // This must be the first feature in the structure chain. This
        // works because descriptor indexing is always enabled.
        deviceFeatures.get<vk::PhysicalDeviceFeatures2>()
    };

    auto result = physicalDevice.createDeviceUnique(chain.get<vk::DeviceCreateInfo>());

    if constexpr (enableVerboseLogging)
    {
        std::cout << "\nLogical device created from physical device \"" << name << "\"\n";

        std::cout << "   Enabled device extensions:\n";
        for (const auto& name : deviceExtensions) {
            std::cout << "    - " << name << "\n";
        }

        std::cout << "   Enabled device features:\n";
        constexpr auto pNextOffset = offsetof(vk::PhysicalDeviceFeatures2, pNext);
        uint8_t* feature = reinterpret_cast<uint8_t*>(&deviceFeatures.get<vk::PhysicalDeviceFeatures2>());
        while (feature != nullptr)
        {
            std::cout << "    - " << vk::to_string(*(vk::StructureType*)feature) << "\n";
            feature = (uint8_t*) *(uint64_t*)(feature + pNextOffset);
        }
    }

    return result;
}

bool vkb::PhysicalDevice::hasSurfaceSupport(vk::SurfaceKHR surface) const
{
    size_t numFamilies = physicalDevice.getQueueFamilyProperties().size();
    for (size_t i = 0; i < numFamilies; i++)
    {
        const bool support = physicalDevice.getSurfaceSupportKHR(i, surface);
        if (support) return true;
    }

    return false;
}

auto vkb::PhysicalDevice::getSwapchainSupport(vk::SurfaceKHR surface) const noexcept
    -> SwapchainSupport
{
    SwapchainSupport result;

    result.surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    result.surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    result.surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    return result;
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
        detectedDevices.emplace_back(PhysicalDevice(device, surface));
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
        && supportsRequiredQueueCapabilities(device);
}

bool vkb::device_helpers::supportsRequiredQueueCapabilities(const PhysicalDevice& device)
{
    const auto& families = device.queueFamilyCapabilities;

    return !families.graphicsCapable.empty()
        && !families.presentationCapable.empty()
        && !families.transferCapable.empty()
        && !families.computeCapable.empty();
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

auto vkb::device_helpers::getRequiredDeviceExtensions() -> std::vector<const char*>
{
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    };
}



namespace std
{
    auto to_string(vkb::QueueType queueType) -> std::string
    {
        switch (queueType)
        {
        case vkb::QueueType::graphics:
            return "Graphics";
            break;
        case vkb::QueueType::compute:
            return "Compute";
            break;
        case vkb::QueueType::transfer:
            return "Transfer";
            break;
        case vkb::QueueType::presentation:
            return "Presentation";
            break;
        case vkb::QueueType::sparseMemory:
            return "Sparse Memory";
            break;
        case vkb::QueueType::protectedMemory:
            return "Protected Memory";
            break;
        case vkb::QueueType::numQueueTypes:
            [[fallthrough]];
        default:
            throw std::logic_error("");
        }
    }
}
