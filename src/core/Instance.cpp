#include "core/Instance.h"

#include "Torch.h"
#include "Window.h"



trc::Instance::Instance(const InstanceCreateInfo& info)
    :
    instance(*::trc::getVulkanInstance()),
    physicalDevice([this] {
        vkb::Surface surface{
            vkb::createSurface(instance, { .windowSize={ 1, 1 }, .windowTitle="" })
        };
        return new vkb::PhysicalDevice(instance, *surface.surface);
    }()),
    device([this, &info] {
        void* deviceFeatureChain{ nullptr };
        auto extensions = info.deviceExtensions;

#ifdef TRC_USE_RAY_TRACING
        // Ray tracing device features
        auto features = physicalDevice->physicalDevice.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR
        >();

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
        if (rayTracingSupported && info.enableRayTracing)
        {
            rayTracingEnabled = true;

            // Add device extensions
            extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
            extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);

            // Add ray tracing features to feature chain
            deviceFeatureChain = features.get<vk::PhysicalDeviceFeatures2>().pNext;
        }
#endif // TRC_USE_RAY_TRACING

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

auto trc::Instance::makeWindow(const WindowCreateInfo& info) -> u_ptr<Window>
{
    return std::make_unique<Window>(*this, info);
}

bool trc::Instance::hasRayTracing() const
{
    return rayTracingEnabled;
}
