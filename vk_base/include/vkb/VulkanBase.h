#pragma once

#include <vector>
#include <functional>

#include "basics/Instance.h"
#include "basics/VulkanDebug.h"
#include "basics/PhysicalDevice.h"
#include "basics/Device.h"
#include "basics/Swapchain.h"
#include "QueueProvider.h"

namespace vkb
{
    struct VulkanInitInfo
    {
        vk::Extent2D windowSize{ 1920, 1080 };
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
        static void onInit(std::function<void(void)> callback);
        static void onDestroy(std::function<void(void)> callback);

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

        static bool isInitialized() noexcept;

        static auto createSurface(vk::Extent2D size) -> Surface;

    public:
        static auto getInstance() noexcept       -> VulkanInstance&;
        static auto getPhysicalDevice() noexcept -> PhysicalDevice&;
        static auto getDevice() noexcept         -> Device&;
        static auto getSwapchain() noexcept      -> Swapchain&;

        static auto getQueueProvider() noexcept  -> QueueProvider&;

    private:
        static inline bool _isInitialized{ false };
        static inline std::vector<std::function<void(void)>> onInitCallbacks;
        static inline std::vector<std::function<void(void)>> onDestroyCallbacks;

        static inline std::unique_ptr<VulkanInstance> instance{ nullptr };

        static inline std::unique_ptr<PhysicalDevice> physicalDevice{ nullptr };
        static inline std::unique_ptr<Device>         device{ nullptr };
        static inline std::unique_ptr<Swapchain>      swapchain{ nullptr };

        static inline std::unique_ptr<QueueProvider>  queueProvider{ nullptr };
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

    /** @brief Shortcut for vkb::VulkanBase::getQueueProvider() */
    inline auto getQueueProvider() noexcept -> QueueProvider& {
        return VulkanBase::getQueueProvider();
    }


    /**
     * @brief Helper for static creation of vulkan objects
     *
     * vulkanStaticInit() is called on the subclass when the vulkan
     * library is initialized.
     */
    template<class Derived>
    class VulkanStaticInitialization
    {
    public:
        // Ensure that the static variable is instantiated
        VulkanStaticInitialization() { _init; }

    private:
        static inline bool _init = []() {
            VulkanBase::onInit([&]() {
                Derived::vulkanStaticInit();
            });
            return true;
        }();
    };


    /**
     * @brief Helper for static destruction of vulkan objects
     *
     * vulkanStaticDestroy() is called on the subclass when the vulkan library
     * is terminated.
     */
    template<class Derived>
    class VulkanStaticDestruction
    {
    public:
        // Ensure that the static variable is instantiated
        VulkanStaticDestruction() { _init; }

    private:
        static inline bool _init = []() {
            VulkanBase::onDestroy([&]() {
                Derived::vulkanStaticDestroy();
            });
            return true;
        }();
    };
} // namespace vkb
