#pragma once

#include <cassert>
#include <vector>
#include <functional>

#include "FrameClock.h"

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
         * @param const FrameClock& frameClock
         */
        FrameSpecific(const FrameClock& frameClock) requires(std::is_default_constructible_v<R>)
            : FrameSpecific(frameClock, [](uint32_t) { return R{}; })
        {}

        FrameSpecific(const FrameClock& frameClock, std::vector<R> objects)
            :
            FrameSpecific(frameClock, [&objects](uint32_t imageIndex) {
                return std::move(objects[imageIndex]);
            })
        {}

        /**
         * @param const FrameClock& frameClock The frameClock
         * @param std::function<R(uint32_t)> func: A constructor function
         *        for the object. Is called for every frame in the frame
         *        clock. Provides the current frame index as argument.
         */
        FrameSpecific(const FrameClock& frameClock, std::function<R(uint32_t)> func)
            : frameClock(&frameClock)
        {
            const uint32_t count{ frameClock.getFrameCount() };
            objects.reserve(count);
            for (uint32_t i = 0; i < count; i++) {
                objects.emplace_back(func(i));
            }
        }

        FrameSpecific(const FrameSpecific&) requires std::copy_constructible<R>
            = default;
        FrameSpecific(FrameSpecific&&) = default;
        ~FrameSpecific() = default;

        FrameSpecific& operator=(const FrameSpecific&) requires std::is_copy_assignable_v<R>
            = default;
        FrameSpecific& operator=(FrameSpecific&&) = default;

        inline auto operator*() -> R& {
            return objects[frameClock->getCurrentFrame()];
        }

        inline auto operator*() const -> const R& {
            return objects[frameClock->getCurrentFrame()];
        }

        inline auto operator->() -> R* {
            return &objects[frameClock->getCurrentFrame()];
        }

        inline auto operator->() const -> const R* {
            return &objects[frameClock->getCurrentFrame()];
        }

        inline auto getFrameClock() const -> const FrameClock& {
            return *frameClock;
        }

        /**
         * Returns the object for the currently 'active' frame.
         *
         * @return R& The object for the current frame
         */
        inline auto get() noexcept -> R& {
            return objects[frameClock->getCurrentFrame()];
        }

        /**
         * Returns the object for the currently 'active' frame.
         *
         * @return const R& The object for the current frame
         */
        inline auto get() const noexcept -> const R& {
            return objects[frameClock->getCurrentFrame()];
        }

        /**
         * @return const R& The object for a specific frame
         */
        inline auto getAt(uint32_t imageIndex) noexcept -> R&
        {
            assert(imageIndex < objects.size());
            return objects[imageIndex];
        }

        /**
         * @return const R& The object for a specific frame
         */
        inline auto getAt(uint32_t imageIndex) const noexcept -> const R&
        {
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
        inline void foreach(std::function<void(R&)> func)
        {
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

        inline auto begin() const {
            return objects.begin();
        }

        inline auto end() const {
            return objects.end();
        }

    private:
        const FrameClock* frameClock;
        std::vector<R> objects;
    };
} // namespace vkb
