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


/*
A VulkanBase that provides a CRTP static interface to manage
objects of the derived class. */
template<class Derived, typename... ConstructArgs>
class VulkanManagedObject : public VulkanBase,
                            private VulkanStaticDestruction<
                                VulkanManagedObject<Derived, ConstructArgs...>
                            >
{
public:
    /**
     * @brief Creates a new Derived
     *
     * Generates a new index for the object.
     * Returns the created object. If the index was already occupied, also
     * returns the object that was previously at the index.
     *
     * @param ConstructArgs&&... args: The constructor arguments
     *
     * @return pair<size_t index, Derived* createdObject>
     */
    [[nodiscard]]
    static auto create(ConstructArgs&&... args) -> Derived&;

    /**
     * @brief Creates a new Derived at a specific index
     *
     * Returns the created object. If the index was already occupied, also
     * returns the object that was previously at the index.
     *
     * @param size_t index A key for the new object.
     * @param ConstructArgs&&... args The constructor arguments
     */
    static auto create(size_t index, ConstructArgs&&... args)
        -> std::pair<Derived&, std::optional<std::unique_ptr<Derived>>>;

    /*
    Returns the stored object at the index.
    Returns nullopt if no object exists at the index.

    @param size_t index: The index to look at

    @return optional<Derived*> The object at the specified index */
    [[nodiscard]]
    static auto find(size_t index) noexcept -> std::optional<Derived*>;

    /*
    Returns the stored object at the index.
    Throws a std::out_of_range if no object exists at that index.

    @param size_t index: The index to look at

    @return Derived* The object at the specified index */
    [[nodiscard]]
    static auto at(size_t index) -> Derived&;

    /*
    Destroys the object at the index.
    Does nothing if no object exists at the index.

    @param size_t index: The index of the object to destroy */
    static void destroy(size_t index);

    /*
    Returns the map-index of the object.
    If the object has not been created with VulkanManagedObject::create()
    ans thus has no index because it is not present in the map, the index
    is UINT64_MAX.

    @return size_t map index */
    [[nodiscard]]
    inline auto getIndex() const noexcept -> size_t {
        return _index;
    }

    static void vulkanStaticDestroy() noexcept {
        _objects.clear();
    }

private:
    static size_t getNextIndex() noexcept;
    static inline std::vector<std::unique_ptr<Derived>> _objects;

    size_t _index{ UINT64_MAX };
};



// create
template<class Derived, typename ...ConstructArgs>
inline auto VulkanManagedObject<Derived, ConstructArgs...>::create(ConstructArgs&&... args) -> Derived&
{
    return create(getNextIndex(), std::forward<ConstructArgs>(args)...).first;
}

// create
template<class Derived, typename ...ConstructArgs>
inline auto VulkanManagedObject<Derived, ConstructArgs...>::create(size_t index, ConstructArgs&&... args)
    -> std::pair<Derived&, std::optional<std::unique_ptr<Derived>>>
{
    if (index >= _objects.size())
    {
        _objects.resize(index + 1);
    }

    bool occupied = find(index).has_value();
    if (occupied)
    {
        auto existing = std::move(_objects[index]);
        Derived& newObj = **_objects.emplace(
            _objects.begin() + index,
            std::make_unique<Derived>(std::forward<ConstructArgs>(args)...)
        );
        newObj._index = index;

        return std::pair<Derived&, std::optional<std::unique_ptr<Derived>>>(
            newObj,
            std::move(existing)
        );
    }

    Derived& newObj = **_objects.emplace(
        _objects.begin() + index,
        std::make_unique<Derived>(std::forward<ConstructArgs>(args)...)
    );
    newObj._index = index;

    return std::pair<Derived&, std::optional<std::unique_ptr<Derived>>>(
        newObj,
        std::nullopt
    );
}

// get
template<class Derived, typename ...ConstructArgs>
inline auto VulkanManagedObject<Derived, ConstructArgs...>::find(size_t index) noexcept
-> std::optional<Derived*>
{
    if (index >= _objects.size() || _objects[index] == nullptr)
        return std::nullopt;
    return &*_objects[index];
}

// at
template<class Derived, typename ...ConstructArgs>
inline auto VulkanManagedObject<Derived, ConstructArgs...>::at(size_t index) -> Derived&
{
    try {
        return *_objects.at(index);
    }
    catch (const std::out_of_range&) {
        throw std::out_of_range("at(): No object found at index " + std::to_string(index) + ".");
    }
}

// destroy
template<class Derived, typename ...ConstructArgs>
inline void VulkanManagedObject<Derived, ConstructArgs...>::destroy(size_t index)
{
    try {
        _objects.at(index) = nullptr;
    }
    catch (const std::out_of_range&) {
        throw std::out_of_range("destroy(): No object found at index " + std::to_string(index) + ".");
    }
}

// getNextIndex
template<class Derived, typename ...ConstructArgs>
inline size_t VulkanManagedObject<Derived, ConstructArgs...>::getNextIndex() noexcept
{
    return _objects.size();
}

} // namespace vkb
