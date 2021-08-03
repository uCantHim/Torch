#include "core/Instance.h"

#include "Torch.h"
#include "Window.h"



auto trc::createDefaultInstance(DefaultInstanceCreateInfo info) -> u_ptr<Instance>
{
    vk::Instance instance{ *getVulkanInstance() };
    vkb::Surface surface{
        vkb::createSurface(instance, { .windowSize={ 1, 1 }, .windowTitle="" })
    };
    auto phys = std::make_unique<vkb::PhysicalDevice>(
        vkb::device_helpers::getOptimalPhysicalDevice(instance, *surface.surface)
    );

    void* deviceFeatureChain{ nullptr };

#ifdef TRC_USE_RAY_TRACING
    // Ray tracing device features
    vk::StructureChain rayTracingDeviceFeatures{
        vk::PhysicalDeviceFeatures2{}, // required for chain validity
        vk::PhysicalDeviceBufferDeviceAddressFeatures{},
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR{},
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{}
    };
    if (info.enableRayTracing)
    {
        // Add device extensions
        info.deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        info.deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        info.deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        info.deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);

        // Add ray tracing features to feature chain
        deviceFeatureChain = &rayTracingDeviceFeatures.get<vk::PhysicalDeviceBufferDeviceAddressFeatures>();
    }
#endif

    auto device = std::make_unique<vkb::Device>(*phys, info.deviceExtensions, deviceFeatureChain);

    return std::make_unique<Instance>(
        InstanceCreateInfo{
            .instance=instance,
            .physicalDevice=std::move(phys),
            .device=std::move(device),
        }
    );
}



trc::Instance::Instance(InstanceCreateInfo&& info)
    :
    instance(info.instance),
    physicalDevice(std::move(info.physicalDevice)),
    device(std::move(info.device)),
    queueManager(*physicalDevice, *device),
    dynamicLoader(std::move(info.dynamicLoader))
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

auto trc::Instance::getQueueManager() -> vkb::QueueManager&
{
    return queueManager;
}

auto trc::Instance::getQueueManager() const -> const vkb::QueueManager&
{
    return queueManager;
}

auto trc::Instance::getDL() -> vk::DispatchLoaderDynamic&
{
    return dynamicLoader;
}

auto trc::Instance::getDL() const -> const vk::DispatchLoaderDynamic&
{
    return dynamicLoader;
}

auto trc::Instance::makeWindow(const WindowCreateInfo& info) const -> u_ptr<Window>
{
    return std::make_unique<Window>(*this, info);
}
