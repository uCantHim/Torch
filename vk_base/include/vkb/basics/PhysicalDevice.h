#pragma once

#include <vector>
#include <array>

#include <vulkan/vulkan.hpp>

namespace vkb
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

    struct QueueCapabilities
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
        -> QueueCapabilities;


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

        /**
         * @brief Create a logical device from the physical device
         */
        auto createLogicalDevice() const -> vk::UniqueDevice;

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


        ///////////////////////////
        // Public device properties

        const vk::PhysicalDevice physicalDevice;

        // Queues
        const std::vector<QueueFamily> queueFamilies;
        const QueueCapabilities queueCapabilities;

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

    namespace device_helpers
    {
        auto findAllPhysicalDevices(vk::Instance instance, vk::SurfaceKHR surface)
            -> std::vector<PhysicalDevice>;

        auto getOptimalPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
            -> PhysicalDevice;

        bool isOptimalDevice(const PhysicalDevice& device);

        /**
         * Required queue families are:
         *  - graphics family
         *  - presentation family
         *  - transfer family
         */
        bool supportsRequiredQueueCapabilities(const PhysicalDevice& device);
        bool supportsRequiredDeviceExtensions(const PhysicalDevice& device);
    }
} // namespace vkb


namespace std
{
    extern auto to_string(vkb::QueueType queueType) -> std::string;
}
