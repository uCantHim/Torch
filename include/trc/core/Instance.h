#pragma once

#include <vkb/basics/Instance.h>
#include <vkb/basics/Device.h>

#include "Types.h"

namespace trc
{
    class Window;
    struct WindowCreateInfo;

    struct InstanceCreateInfo
    {
        bool enableRayTracing{ false };
        std::vector<const char*> deviceExtensions;
    };

    class Instance
    {
    public:
        Instance(const Instance&) = delete;
        Instance(Instance&&) noexcept = delete;
        auto operator=(const Instance&) -> Instance& = delete;
        auto operator=(Instance&&) noexcept -> Instance& = delete;

        explicit Instance(const InstanceCreateInfo& info = {});
        ~Instance();

        auto getVulkanInstance() const -> vk::Instance;

        auto getPhysicalDevice() -> vkb::PhysicalDevice&;
        auto getPhysicalDevice() const -> const vkb::PhysicalDevice&;
        auto getDevice() -> vkb::Device&;
        auto getDevice() const -> const vkb::Device&;
        auto getQueueManager() -> vkb::QueueManager&;
        auto getQueueManager() const -> const vkb::QueueManager&;
        auto getDL() -> vk::DispatchLoaderDynamic&;
        auto getDL() const -> const vk::DispatchLoaderDynamic&;

        auto makeWindow(const WindowCreateInfo& info) const -> u_ptr<Window>;

        bool hasRayTracing() const;

    private:
        vk::Instance instance;
        u_ptr<vkb::PhysicalDevice> physicalDevice;
        u_ptr<vkb::Device> device;
        vkb::QueueManager queueManager;

        vk::DispatchLoaderDynamic dynamicLoader;

        bool rayTracingEnabled{ false };
    };
} // namespace trc
