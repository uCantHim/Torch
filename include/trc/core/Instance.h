#pragma once

#include <string>
#include <vector>

#include <vkb/VulkanInstance.h>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"
#include "TypeErasedStructureChain.h"

namespace vkb
{
    class PhysicalDevice;
    class Device;
}

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

    /**
     * @brief Create a Vulkan instance with default settings
     */
    auto makeDefaultTorchVulkanInstance(const std::string& appName = "A Torch application",
                                        ui32 appVersion = 0)
        -> u_ptr<vkb::VulkanInstance>;

    /**
     * @brief The most basic structure needed to set up Torch
     *
     * This class contains a Vulkan instance as well as a device.
     */
    class Instance
    {
    public:
        Instance(const Instance&) = delete;
        auto operator=(const Instance&) -> Instance& = delete;

        Instance(Instance&&) noexcept = default;
        auto operator=(Instance&&) noexcept -> Instance& = default;

        explicit Instance(const InstanceCreateInfo& info = {});

        /**
         * @brief Create a Torch instance from a user-created vk::Instance
         *
         * @param const InstanceCreateInfo&
         * @param vk::Instance instance The Vulkan instance object used.
         *        Warning: will not be destroyed when the trc::Instance
         *        object is destroyed!
         */
        Instance(const InstanceCreateInfo& info, vk::Instance instance);
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
        /**
         * @return The created device and a flag indicating whether ray
         *         tracing is supported by the hardware.
         */
        static auto makeDevice(const InstanceCreateInfo& info,
                          const vkb::PhysicalDevice& physicalDevice)
            -> std::pair<u_ptr<vkb::Device>, bool>;

        bool hasRayTracingFeatures{ false };

        u_ptr<vkb::VulkanInstance> optionalLocalInstance;

        vk::Instance instance;
        u_ptr<vkb::PhysicalDevice> physicalDevice;
        u_ptr<vkb::Device> device;

        vk::DispatchLoaderDynamic dynamicLoader;
    };
} // namespace trc
