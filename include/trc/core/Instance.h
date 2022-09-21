#pragma once

namespace vkb {
    class VulkanInstance;
    class PhysicalDevice;
    class Device;
}

#include "trc/VulkanInclude.h"
#include "trc/Types.h"
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

        /**
         * @brief Create a Torch instance from a user-created vk::Instance
         *
         * @param const InstanceCreateInfo&
         * @param vk::Instance instance The Vulkan instance object used.
         *        If this is VK_NULL_HANDLE, a new Vulkan instance is
         *        created automatically.
         *        Warning: will not be destroyed when the trc::Instance
         *        object is destroyed!
         */
        Instance(const InstanceCreateInfo& info = {}, vk::Instance instance = VK_NULL_HANDLE);
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

        u_ptr<vkb::VulkanInstance> optionalLocalInstance;

        vk::Instance instance;
        u_ptr<vkb::PhysicalDevice> physicalDevice;
        u_ptr<vkb::Device> device;

        vk::DispatchLoaderDynamic dynamicLoader;
    };
} // namespace trc
