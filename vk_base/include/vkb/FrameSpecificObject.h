#pragma once

#include <vector>
#include <functional>

#include "VulkanBase.h"

namespace vkb
{

template<class R>
class FrameSpecificObject : public VulkanBase, private Swapchain::FrameCounter
{
public:
    /**
     * Default constructor.
     * Only available when R is default constructible.
     */
    FrameSpecificObject()// requires(std::is_default_constructible_v<R>)
        : FrameSpecificObject([](uint32_t) { return R{}; })
    {
    }; // Semicolon for linting purposes until C++20 is supported

    /**
     * Full-initialization constructor.
     *
     * @param std::function<R(uint32_t)> func: A constructor function for the
     * object. Is called for every frame in the swapchain. Provies the current
     * frame index as argument.
     */
    explicit FrameSpecificObject(std::function<R(uint32_t)> func) {
        uint32_t count{ getSwapchain().getFrameCount() };
        for (uint32_t i = 0; i < count; i++) {
            objects.push_back(func(i));
        }
    }

    FrameSpecificObject(const FrameSpecificObject&) = delete;
    FrameSpecificObject(FrameSpecificObject&&) = default;
    ~FrameSpecificObject() = default;

    FrameSpecificObject& operator=(const FrameSpecificObject&) = delete;
    FrameSpecificObject& operator=(FrameSpecificObject&&) = default;

    inline auto operator*() -> R& {
        return objects[getCurrentFrame()];
    }

    inline auto operator->() -> R* {
        return &objects[getCurrentFrame()];
    }

    /**
     * Returns the object for the currently 'active' frame.
     *
     * @return R& The object for the current frame
     */
    [[nodiscard]]
    inline auto get() noexcept -> R& {
        return objects[getCurrentFrame()];
    }

    /**
     * Returns the object for the currently 'active' frame.
     *
     * @return const R& The object for the current frame
     */
    [[nodiscard]]
    inline auto get() const noexcept -> const R& {
        return objects[getCurrentFrame()];
    }

    /**
     * @return const R& The object for a specific frame
     */
    [[nodiscard]]
    inline auto getAt(uint32_t imageIndex) const noexcept -> const R& {
        assert(imageIndex < objects.size());
        return objects[imageIndex];
    }

    /**
     * Applies a function to every object. Provides the currently processed
     * object to the function.
     *
     * @param std::function<void(R&)> func The mapped function, called for
     *                                        every object.
     */
    inline void foreach(std::function<void(R&)> func) {
        for (auto& object : objects) {
            func(object);
        }
    }


private:
    std::vector<R> objects;
};


using CmdBuf = FrameSpecificObject<vk::UniqueCommandBuffer>;

} // namespace vkb
