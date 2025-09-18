#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "trc/VulkanInclude.h"

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

    auto to_string(QueueType queueType) -> std::string;

    using QueueFamilyIndex = uint32_t;

    struct QueueFamily
    {
        static constexpr size_t kNumQueueTypes = static_cast<size_t>(QueueType::numQueueTypes);

        QueueFamily() = default;
        QueueFamily(uint32_t _index, uint32_t _queueCount,
                    std::array<bool, kNumQueueTypes> _capabilities)
            :
            index(_index),
            queueCount(_queueCount),
            capabilities(_capabilities)
        {}

        QueueFamilyIndex index;
        uint32_t queueCount;

        bool isCapable(QueueType type) const noexcept {
            return capabilities[static_cast<size_t>(type)];
        }

    private:
        std::array<bool, kNumQueueTypes> capabilities;
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

    /**
     * @brief Find all queue families on a physical device.
     */
    auto getQueueFamilies(vk::PhysicalDevice,
                          std::optional<vk::SurfaceKHR> surface = {})
        -> std::vector<QueueFamily>;

    /**
     * @brief Categorize a list of queue families by their capabilities.
     */
    auto sortByCapabilities(const std::vector<QueueFamily>& families)
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
         * @brief Create a physical device object
         *
         * Requires a surface because it has to query for swapchain and
         * presentation support.
         *
         * @param surface Either std::nullopt to create a physical device
         *                without surface/present support, or a valid handle.
         */
        PhysicalDevice(vk::Instance instance,
                       vk::PhysicalDevice device,
                       std::optional<vk::SurfaceKHR> surface);

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
         * @param deviceExtensions Additional device extensions to enable.
         * @param extraPhysicalDeviceFeatureChain Additional device
         *                                        features to enable.
         */
        auto makeLogicalDevice(std::vector<const char*> deviceExtensions = {},
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

        const vk::Instance instance;
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

    /**
     * List all available physical devices.
     *
     * @param surface Specify a surface to check for present capabilities
     *                on queue families of detected devices. If none is
     *                specified, queues will not be marked as 'presentation
     *                capable', even though they might be. This is a technical
     *                requirement imposed by Vulkan.
     */
    auto findAllPhysicalDevices(vk::Instance instance,
                                std::optional<vk::SurfaceKHR> surface = {})
        -> std::vector<PhysicalDevice>;

    /**
     * Try to find a physical device that supports all required extensions
     * and queue capabilities.
     *
     * @param surface Specify a surface to check for present capabilities
     *                on queue families of detected devices. If none is
     *                specified, queues will not be marked as 'presentation
     *                capable', even though they might be. This is a technical
     *                requirement imposed by Vulkan.
     *
     * @return std::nullopt if no such device can be found.
     */
    auto findOptimalPhysicalDevice(vk::Instance instance,
                                   std::optional<vk::SurfaceKHR> surface = {})
        -> std::optional<PhysicalDevice>;

    /**
     * Try to find an optimal physical device that supports all required
     * extensions and queue capabilities, but fall back to an inferior one
     * if no such device can be found.
     *
     * @param surface Specify a surface to check for present capabilities
     *                on queue families of detected devices. If none is
     *                specified, queues will not be marked as 'presentation
     *                capable', even though they might be. This is a technical
     *                requirement imposed by Vulkan.
     *
     * @return std::nullopt if no physical device can be found at all.
     */
    auto findBestPhysicalDevice(vk::Instance instance,
                                std::optional<vk::SurfaceKHR> surface = {})
        -> std::optional<PhysicalDevice>;

    namespace device_helpers
    {
        bool isOptimalDevice(const PhysicalDevice& device, bool requirePresentation);

        /**
         * Required queue families are:
         *  - graphics family
         *  - compute family
         *  - transfer family
         *  - presentation family if `requirePresentation == true`
         */
        bool supportsRequiredQueueCapabilities(const PhysicalDevice& device,
                                               bool requirePresentation);
        bool supportsRequiredDeviceExtensions(const PhysicalDevice& device,
                                              bool requirePresentation);

        /**
         * @brief Basic extensions that are always loaded.
         */
        auto getRequiredDeviceExtensions(bool requirePresentation) -> std::vector<const char*>;
    }
} // namespace trc


namespace std
{
}
