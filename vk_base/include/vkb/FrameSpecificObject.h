#pragma once

#include <vector>
#include <functional>

#include "basics/Swapchain.h"

namespace vkb
{
    template<class R>
    class FrameSpecific
    {
    public:
        /**
         * Default constructor.
         * Only available when R is default constructible.
         *
         * @param const Swapchain& swapchain
         */
        FrameSpecific(const Swapchain& swapchain) requires(std::is_default_constructible_v<R>)
            : FrameSpecific(swapchain, [](uint32_t) { return R{}; })
        {}

        FrameSpecific(const Swapchain& swapchain, std::vector<R> objects)
            :
            FrameSpecific(swapchain, [&objects](uint32_t imageIndex) {
                return std::move(objects[imageIndex]);
            })
        {}

        /**
         * @param const Swapchain& swapchain The swapchain
         * @param std::function<R(uint32_t)> func: A constructor function
         * for the object. Is called for every frame in the swapchain.
         * Provides the current frame index as argument.
         */
        FrameSpecific(const Swapchain& swapchain, std::function<R(uint32_t)> func)
            : swapchain(&swapchain)
        {
            uint32_t count{ swapchain.getFrameCount() };
            objects.reserve(count);
            for (uint32_t i = 0; i < count; i++) {
                objects.push_back(func(i));
            }
        }

        FrameSpecific(const FrameSpecific&) = delete;
        FrameSpecific(FrameSpecific&&) = default;
        ~FrameSpecific() = default;

        FrameSpecific& operator=(const FrameSpecific&) = delete;
        FrameSpecific& operator=(FrameSpecific&&) = default;

        inline auto operator*() -> R& {
            return objects[swapchain->getCurrentFrame()];
        }

        inline auto operator*() const -> const R& {
            return objects[swapchain->getCurrentFrame()];
        }

        inline auto operator->() -> R* {
            return &objects[swapchain->getCurrentFrame()];
        }

        inline auto operator->() const -> const R* {
            return &objects[swapchain->getCurrentFrame()];
        }

        /**
         * Returns the object for the currently 'active' frame.
         *
         * @return R& The object for the current frame
         */
        inline auto get() noexcept -> R& {
            return objects[swapchain->getCurrentFrame()];
        }

        /**
         * Returns the object for the currently 'active' frame.
         *
         * @return const R& The object for the current frame
         */
        inline auto get() const noexcept -> const R& {
            return objects[swapchain->getCurrentFrame()];
        }

        /**
         * @return const R& The object for a specific frame
         */
        inline auto getAt(uint32_t imageIndex) noexcept -> R& {
            assert(imageIndex < objects.size());
            return objects[imageIndex];
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

        inline auto begin() {
            return objects.begin();
        }

        inline auto end() {
            return objects.end();
        }

    private:
        const Swapchain* swapchain;
        std::vector<R> objects;
    };
} // namespace vkb
