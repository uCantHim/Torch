#pragma once

#include <array>
#include <vector>

#include "trc/VulkanInclude.h"
#include "trc/base/VulkanDebug.h"

namespace trc
{
    /* A queue type is the capability of a queue to perform a certain task.
    Queue types are defined by the queue family that a queue belongs to.
    Queues can have multiple queue types. */
    enum class QueueType
    {
        graphics, compute,
        transfer,
        sparseMemory, protectedMemory,
        presentation,
        numQueueTypes
    };

    using QueueFamilyIndex = uint32_t;

    struct QueueFamily
    {
        QueueFamily() = default;
        QueueFamily(uint32_t _index, uint32_t _queueCount,
                    std::array<bool, static_cast<size_t>(QueueType::numQueueTypes)> capabilities)
            :
            index(_index),
            queueCount(_queueCount),
            capabilities(capabilities.begin(), capabilities.end())
        {}

        QueueFamilyIndex index;
        uint32_t queueCount;

        bool isCapable(QueueType type) const noexcept {
            return capabilities[static_cast<size_t>(type)];
        }

    private:
        std::vector<bool> capabilities;
    };

    struct QueueFamilyCapabilities
    {
        std::vector<QueueFamily> graphicsCapable;
        std::vector<QueueFamily> computeCapable;
        std::vector<QueueFamily> transferCapable;
        std::vector<QueueFamily> sparseMemoryCapable;
        std::vector<QueueFamily> protectedMemoryCapable;
        std::vector<QueueFamily> presentationCapable;
    };

    extern auto getQueueFamilies(vk::PhysicalDevice, vk::SurfaceKHR surface)
        -> std::vector<QueueFamily>;
    extern auto sortByCapabilities(const std::vector<QueueFamily>& families)
        -> QueueFamilyCapabilities;


    /**
     * @brief A physical vulkan-capable device
     */
    class PhysicalDevice
    {
    public:
        struct SwapchainSupport
        {
            vk::SurfaceCapabilitiesKHR surfaceCapabilities;
            std::vector<vk::SurfaceFormatKHR> surfaceFormats;
            std::vector<vk::PresentModeKHR> surfacePresentModes;
        };

        /**
         * @brief Create optimal physical device
         *
         * Uses several restrictions to search for an optimal physical
         * device in the system and uses that. Throws a std::runtime_error
         * if no such device exists.
         *
         * @throw std::runtime_error if no appropriate device can be found.
         */
        PhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface);

        /**
         * @brief Create a physical device object
         *
         * Requires a surface because it has to query for swapchain and
         * presentation support.
         */
        PhysicalDevice(vk::PhysicalDevice device, vk::SurfaceKHR surface);

        PhysicalDevice(const PhysicalDevice&) = default;
        PhysicalDevice(PhysicalDevice&&) noexcept = default;
        ~PhysicalDevice() = default;

        PhysicalDevice& operator=(const PhysicalDevice&) = delete;
        PhysicalDevice& operator=(PhysicalDevice&&) noexcept = delete;

        auto operator->() const noexcept -> const vk::PhysicalDevice*;
        auto operator*() const noexcept -> vk::PhysicalDevice;

        /**
         * @brief Create a logical device from the physical device
         *
         * Some basic extensions that are required to be supported by the
         * specification will be enabled by default.
         *
         * @param std::vector<const char*> deviceExtensions Extensions to
         *        enable on the device.
         * @param void* extraPhysicalDeviceFeatureChain Additional chained
         *        device features to enable on the logical device. This
         *        pointer will be set as pNext of the end of vkb's default
         *        feature chain.
         */
        auto createLogicalDevice(std::vector<const char*> deviceExtensions = {},
                                 void* extraPhysicalDeviceFeatureChain = nullptr) const
            -> vk::UniqueDevice;

        /**
         * Determines whether the device has any queue family with
         * presentation support for a specific surface.
         *
         * @return bool
         */
        bool hasSurfaceSupport(vk::SurfaceKHR surface) const;

        /**
         * @return SwapchainSupport Detailed information about the device's
         *         support for a specific surface
         */
        auto getSwapchainSupport(vk::SurfaceKHR surface) const noexcept
            -> SwapchainSupport;

        /**
         * @brief Find the index of a memory type with specific properties
         *
         * @param uint32_t memoryTypeBits The memory type bits in a queried
         *                                vk::MemoryRequirements structure
         * @param vk::MemoryPropertyFlags properties Required memory
         *                                           properties
         *
         * @return uint32_t The index of the searched memory type
         */
        uint32_t findMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags properties) const;

        /**
         * @brief Query a feature from the device
         *
         * @tparam T The type of feature to query;
         *           e.g. vk::PhysicalDeviceRayTracingPipelineFeaturesKHR
         *
         * @return T
         */
        template<typename T>
        inline auto getFeature() const -> T
        {
            return physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, T>()
                .template get<T>();
        }


        ///////////////////////////
        // Public device properties

        const vk::PhysicalDevice physicalDevice;

        // Queues
        const std::vector<QueueFamily> queueFamilies;
        const QueueFamilyCapabilities queueFamilyCapabilities;

        // Physical device properties
        const std::vector<vk::ExtensionProperties> supportedExtensions;
        const vk::PhysicalDeviceProperties properties;
        const vk::PhysicalDeviceFeatures features;

        // Memory
        const vk::PhysicalDeviceMemoryProperties memoryProperties;

        // Other
        const std::string name;
        const vk::PhysicalDeviceType type;
        const std::string typeString;
    };

    auto findAllPhysicalDevices(vk::Instance instance, vk::SurfaceKHR surface)
        -> std::vector<PhysicalDevice>;

    auto findOptimalPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
        -> PhysicalDevice;

    namespace device_helpers
    {
        bool isOptimalDevice(const PhysicalDevice& device);

        /**
         * Required queue families are:
         *  - graphics family
         *  - presentation family
         *  - transfer family
         */
        bool supportsRequiredQueueCapabilities(const PhysicalDevice& device);
        bool supportsRequiredDeviceExtensions(const PhysicalDevice& device);

        /**
         * @brief Basic extensions that are always loaded.
         */
        auto getRequiredDeviceExtensions() -> std::vector<const char*>;
    }
} // namespace trc


namespace std
{
    auto to_string(trc::QueueType queueType) -> std::string;
}
