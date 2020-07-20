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
        FrameSpecificObject() requires(std::is_default_constructible_v<R>)
            : FrameSpecificObject([](uint32_t) { return R{}; })
        {
        }

        /**
         * Full-initialization constructor.
         *
         * @param std::function<R(uint32_t)> func: A constructor function for the
         * object. Is called for every frame in the swapchain. Provies the current
         * frame index as argument.
         */
        explicit FrameSpecificObject(std::function<R(uint32_t)> func)
        {
            uint32_t count{ getSwapchain().getFrameCount() };
            objects.reserve(count);
            for (uint32_t i = 0; i < count; i++) {
                objects.push_back(func(i));
            }
        }

        explicit FrameSpecificObject(std::vector<R> objects)
            :
            FrameSpecificObject([&objects](uint32_t imageIndex) {
                return std::move(objects[imageIndex]);
            })
        {}

        FrameSpecificObject(const FrameSpecificObject&) = delete;
        FrameSpecificObject(FrameSpecificObject&&) = default;
        ~FrameSpecificObject() = default;

        FrameSpecificObject& operator=(const FrameSpecificObject&) = delete;
        FrameSpecificObject& operator=(FrameSpecificObject&&) = default;

        inline auto operator*() -> R& {
            return objects[getCurrentFrame()];
        }

        inline auto operator*() const -> const R& {
            return objects[getCurrentFrame()];
        }

        inline auto operator->() -> R* {
            return &objects[getCurrentFrame()];
        }

        inline auto operator->() const -> const R* {
            return &objects[getCurrentFrame()];
        }

        /**
         * Returns the object for the currently 'active' frame.
         *
         * @return R& The object for the current frame
         */
        inline auto get() noexcept -> R& {
            return objects[getCurrentFrame()];
        }

        /**
         * Returns the object for the currently 'active' frame.
         *
         * @return const R& The object for the current frame
         */
        inline auto get() const noexcept -> const R& {
            return objects[getCurrentFrame()];
        }

        /**
         * @return const R& The object for a specific frame
         */
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
} // namespace vkb
