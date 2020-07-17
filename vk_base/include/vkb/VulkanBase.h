#pragma once

#include <vector>
#include <functional>

#include "basics/Instance.h"
#include "basics/VulkanDebug.h"
#include "basics/PhysicalDevice.h"
#include "basics/Device.h"
#include "basics/Swapchain.h"
#include "PoolProvider.h"

namespace vkb
{
    struct VulkanInitInfo
    {
        vk::Extent2D windowSize{ 1920, 1080 };
    };

    void vulkanInit(const VulkanInitInfo& initInfo);
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

        static void init(const VulkanInitInfo& initInfo);
        static void destroy();

        static bool isInitialized() noexcept;

        static auto createSurface(vk::Extent2D size) -> Surface;

    public:
        static auto getInstance() noexcept       -> VulkanInstance&;
        static auto getPhysicalDevice() noexcept -> PhysicalDevice&;
        static auto getDevice() noexcept         -> Device&;
        static auto getSwapchain() noexcept      -> Swapchain&;

        static auto getQueueProvider() noexcept  -> QueueProvider&;
        static auto getPoolProvider() noexcept   -> PoolProvider&;

    private:
        static inline bool _isInitialized{ false };
        static inline std::vector<std::function<void(void)>> onInitCallbacks;
        static inline std::vector<std::function<void(void)>> onDestroyCallbacks;

        static inline std::unique_ptr<VulkanInstance> instance{ nullptr };

        static inline std::unique_ptr<PhysicalDevice> physicalDevice{ nullptr };
        static inline std::unique_ptr<Device>         device{ nullptr };
        static inline std::unique_ptr<Swapchain>      swapchain{ nullptr };

        static inline std::unique_ptr<QueueProvider>  queueProvider{ nullptr };
        static inline std::unique_ptr<PoolProvider>   poolProvider{ nullptr };
    };


    inline auto getInstance() noexcept -> VulkanInstance& {
        return VulkanBase::getInstance();
    }

    inline auto getPhysicalDevice() noexcept -> PhysicalDevice& {
        return VulkanBase::getPhysicalDevice();
    }

    inline auto getDevice() noexcept -> Device& {
        return VulkanBase::getDevice();
    }

    inline auto getSwapchain() noexcept -> Swapchain& {
        return VulkanBase::getSwapchain();
    }

    inline auto getQueueProvider() noexcept -> QueueProvider& {
        return VulkanBase::getQueueProvider();
    }

    inline auto getPoolProvider() noexcept -> PoolProvider& {
        return VulkanBase::getPoolProvider();
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
