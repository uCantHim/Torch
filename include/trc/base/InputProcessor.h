#pragma once

#include <cstdint>

#include "trc/base/event/Keys.h"

namespace trc
{
    class Swapchain;

    /**
     * @brief An interface for user input processing strategies
     */
    class InputProcessor
    {
    public:
        virtual ~InputProcessor() noexcept = default;

        virtual void onCharInput(Swapchain&, uint32_t charcode) = 0;
        virtual void onKeyInput(Swapchain&, Key key, InputAction action, KeyModFlags mods) = 0;
        virtual void onMouseInput(Swapchain&, MouseButton button, InputAction action, KeyModFlags mods) = 0;
        virtual void onMouseMove(Swapchain&, double x, double y) = 0;
        virtual void onMouseScroll(Swapchain&, double xOffset, double yOffset) = 0;
        virtual void onWindowClose(Swapchain&) = 0;
    };

    /**
     * @brief An input processor that does nothing.
     */
    class NullInputProcessor : public InputProcessor
    {
    public:
        void onCharInput(Swapchain&, uint32_t) override {}
        void onKeyInput(Swapchain&, Key, InputAction, KeyModFlags) override {}
        void onMouseInput(Swapchain&, MouseButton, InputAction, KeyModFlags) override {}
        void onMouseMove(Swapchain&, double, double) override {}
        void onMouseScroll(Swapchain&, double, double) override {}
        void onWindowClose(Swapchain&) override {}
    };
} // namespace trc
