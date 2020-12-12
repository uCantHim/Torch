#pragma once

#include <vector>
#include <functional>

#include "basics/Instance.h"
#include "basics/VulkanDebug.h"
#include "basics/PhysicalDevice.h"
#include "basics/Device.h"
#include "basics/Swapchain.h"

namespace vkb
{
    struct SurfaceCreateInfo
    {
        vk::Extent2D windowSize{ 1920, 1080 };
        std::string windowTitle;
    };

    struct VulkanInitInfo
    {
        SurfaceCreateInfo surfaceCreateInfo;

        // Creates instance, device, and swapchain if true
        bool createResources{ true };
    };

    /**
     * @brief Initializes Vulkan functionality
     *
     * You can create Vulkan objects (device, swapchain, ...) yourself if
     * you want to, this is just for convenience.
     */
    void vulkanInit(const VulkanInitInfo& initInfo = {});

    /**
     * @brief Destroy all Vulkan-related objects
     */
    void vulkanTerminate();

    auto createSurface(VulkanInstance& instance, SurfaceCreateInfo createInfo) -> Surface;

    /**
     * @brief Provides access to common vulkan access points
     *
     * Inherit from this to get static access to the essential building blocks:
     *
     *  - Window: A window
     *  - Device: A logical device abstraction. Lets you access the actual
     *               vk::Device.
     *
     * Also provides two higher-level services:
     *
     *  - PoolProvider: Provides convenient access to command buffer pools
     */
    class VulkanBase
    {
    public:
        /**
         * @brief Initializes Vulkan functionality
         *
         * You can create Vulkan objects (device, swapchain, ...) yourself if
         * you want to, this is just for convenience.
         */
        static void init(const VulkanInitInfo& initInfo = {});

        /**
         * @brief Destroy all Vulkan-related objects
         */
        static void destroy();

    public:
        static auto getInstance() noexcept       -> VulkanInstance&;
        static auto getPhysicalDevice() noexcept -> PhysicalDevice&;
        static auto getDevice() noexcept         -> Device&;
        static auto getSwapchain() noexcept      -> Swapchain&;

    private:
        static inline bool isInitialized{ false };

        static inline std::unique_ptr<VulkanInstance> instance{ nullptr };
        static inline std::unique_ptr<PhysicalDevice> physicalDevice{ nullptr };
        static inline std::unique_ptr<Device>         device{ nullptr };
        static inline std::unique_ptr<Swapchain>      swapchain{ nullptr };
    };


    /** @brief Shortcut for vkb::VulkanBase::getInstance() */
    inline auto getInstance() noexcept -> VulkanInstance& {
        return VulkanBase::getInstance();
    }

    /** @brief Shortcut for vkb::VulkanBase::getPhysicalDevice() */
    inline auto getPhysicalDevice() noexcept -> PhysicalDevice& {
        return VulkanBase::getPhysicalDevice();
    }

    /** @brief Shortcut for vkb::VulkanBase::getDevice() */
    inline auto getDevice() noexcept -> Device& {
        return VulkanBase::getDevice();
    }

    /** @brief Shortcut for vkb::VulkanBase::getSwapchain() */
    inline auto getSwapchain() noexcept -> Swapchain& {
        return VulkanBase::getSwapchain();
    }
} // namespace vkb
