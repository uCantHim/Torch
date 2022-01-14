#pragma once

namespace vkb {
    class VulkanInstance;
    class PhysicalDevice;
    class Device;
}

#include "../Types.h"
#include "TypeErasedStructureChain.h"

namespace trc
{
    class Window;
    struct WindowCreateInfo;

    struct InstanceCreateInfo
    {
        /** If true, load all required ray tracing extensions and features */
        bool enableRayTracing{ false };

        /** Additional device extensions to enable */
        std::vector<const char*> deviceExtensions;

        /** Additional device features to enable */
        TypeErasedStructureChain deviceFeatures;
    };

    class Instance
    {
    public:
        Instance(const Instance&) = delete;
        auto operator=(const Instance&) -> Instance& = delete;

        Instance(Instance&&) noexcept = default;
        auto operator=(Instance&&) noexcept -> Instance& = default;

        explicit Instance(const InstanceCreateInfo& info = {});
        ~Instance();

        auto getVulkanInstance() const -> vk::Instance;

        auto getPhysicalDevice() -> vkb::PhysicalDevice&;
        auto getPhysicalDevice() const -> const vkb::PhysicalDevice&;
        auto getDevice() -> vkb::Device&;
        auto getDevice() const -> const vkb::Device&;
        auto getDL() -> vk::DispatchLoaderDynamic&;
        auto getDL() const -> const vk::DispatchLoaderDynamic&;

        auto makeWindow() -> u_ptr<Window>;
        auto makeWindow(const WindowCreateInfo& info) -> u_ptr<Window>;

        bool hasRayTracing() const;

    private:
        bool rayTracingEnabled{ false };

        vk::Instance instance;
        u_ptr<vkb::PhysicalDevice> physicalDevice;
        u_ptr<vkb::Device> device;

        vk::DispatchLoaderDynamic dynamicLoader;
    };
} // namespace trc
