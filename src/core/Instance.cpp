#include "core/Instance.h"

#include <vkb/VulkanInstance.h>
#include <vkb/Device.h>

#include "Window.h"



trc::Instance::Instance(const InstanceCreateInfo& info, vk::Instance _instance)
    :
    // Create a new VkInstance if the _instance argument is VK_NULL_HANDLE
    optionalLocalInstance(_instance ? nullptr : new vkb::VulkanInstance),
    instance(_instance ? _instance : **optionalLocalInstance),

    physicalDevice([instance=this->instance] {
        vkb::Surface surface = vkb::makeSurface(
            instance,
            { .windowSize={ 1, 1 }, .windowTitle="", .hidden=true }
        );
        return new vkb::PhysicalDevice(instance, *surface.surface);
    }()),
    device([this, &info] {
        void* deviceFeatureChain{ nullptr };
        auto extensions = info.deviceExtensions;

        // Required device extensions
        extensions.emplace_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

        // Ray tracing device features
        auto features = physicalDevice->physicalDevice.getFeatures2<
            vk::PhysicalDeviceFeatures2,

            // Misc
            vk::PhysicalDeviceSynchronization2Features,   // Vulkan 1.3
            vk::PhysicalDeviceTimelineSemaphoreFeatures,  // Vulkan 1.2

            // Ray tracing
            vk::PhysicalDeviceBufferDeviceAddressFeatures,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR
        >();

        // Append user provided features to the end
        void* userPtr = info.deviceFeatures.getPNext();
        features.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>().setPNext(userPtr);

        ///////////////////////////////
        // Test for ray tracing support
        auto as = features.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
        auto ray = features.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();

        if constexpr (vkb::enableVerboseLogging)
        {
            std::cout << "\nQuerying ray tracing support:\n";
            std::cout << std::boolalpha
                << "   Acceleration structure: " << (bool)as.accelerationStructure << "\n"
                << "   Acceleration structure host commands: "
                    << (bool)as.accelerationStructureHostCommands << "\n"
                << "   Ray Tracing pipeline: " << (bool)ray.rayTracingPipeline << "\n"
                << "   Trace rays indirect: " << (bool)ray.rayTracingPipelineTraceRaysIndirect << "\n"
                << "   Ray traversal primitive culling: " << (bool)ray.rayTraversalPrimitiveCulling << "\n";
        }

        const bool rayTracingSupported = as.accelerationStructure && ray.rayTracingPipeline;
        const bool enableRayTracing = info.enableRayTracing && rayTracingSupported;
        if (!enableRayTracing)
        {
            features.unlink<vk::PhysicalDeviceBufferDeviceAddressFeatures>();
            features.unlink<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
            features.unlink<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
        }
        else
        {
            this->rayTracingEnabled = true;

            // Add device extensions
            extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
            extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        }

        // Require remaining features from device
        deviceFeatureChain = features.get<vk::PhysicalDeviceFeatures2>().pNext;

        return new vkb::Device(*physicalDevice, extensions, deviceFeatureChain);
    }()),
    dynamicLoader(instance, vkGetInstanceProcAddr)
{
}

trc::Instance::~Instance()
{
    getDevice()->waitIdle();
}

auto trc::Instance::getVulkanInstance() const -> vk::Instance
{
    return instance;
}

auto trc::Instance::getPhysicalDevice() -> vkb::PhysicalDevice&
{
    return *physicalDevice;
}

auto trc::Instance::getPhysicalDevice() const -> const vkb::PhysicalDevice&
{
    return *physicalDevice;
}

auto trc::Instance::getDevice() -> vkb::Device&
{
    return *device;
}

auto trc::Instance::getDevice() const -> const vkb::Device&
{
    return *device;
}

auto trc::Instance::getDL() -> vk::DispatchLoaderDynamic&
{
    return dynamicLoader;
}

auto trc::Instance::getDL() const -> const vk::DispatchLoaderDynamic&
{
    return dynamicLoader;
}

auto trc::Instance::makeWindow() -> u_ptr<Window>
{
    return makeWindow({});
}

auto trc::Instance::makeWindow(const WindowCreateInfo& info) -> u_ptr<Window>
{
    return std::make_unique<Window>(*this, info);
}

bool trc::Instance::hasRayTracing() const
{
    return rayTracingEnabled;
}
