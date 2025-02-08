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
        InputProcessor(const InputProcessor&) = delete;
        InputProcessor(InputProcessor&&) noexcept = delete;
        InputProcessor& operator=(const InputProcessor&) = delete;
        InputProcessor& operator=(InputProcessor&&) noexcept = delete;

        InputProcessor() = default;
        virtual ~InputProcessor() noexcept = default;

        virtual void onCharInput(Swapchain&, uint32_t charcode) = 0;
        virtual void onKeyInput(Swapchain&, Key key, InputAction action, KeyModFlags mods) = 0;
        virtual void onMouseEnter(Swapchain&, bool enter) = 0;
        virtual void onMouseInput(Swapchain&, MouseButton button, InputAction action, KeyModFlags mods) = 0;
        virtual void onMouseMove(Swapchain&, double x, double y) = 0;
        virtual void onMouseScroll(Swapchain&, double xOffset, double yOffset) = 0;

        virtual void onWindowFocus(Swapchain&, bool focus) =  0;
        virtual void onWindowResize(Swapchain&, uint newX, uint newY) =  0;
        virtual void onWindowMove(Swapchain&, int newX, int newY) =  0;
        virtual void onWindowClose(Swapchain&) = 0;

        /**
         * Called when a window's content has become damaged and needs to be
         * refreshed.
         */
        virtual void onWindowRefresh(Swapchain&) =  0;
    };

    /**
     * @brief An input processor that does nothing.
     */
    class NullInputProcessor : public InputProcessor
    {
    public:
        void onCharInput(Swapchain&, uint32_t) override {}
        void onKeyInput(Swapchain&, Key, InputAction, KeyModFlags) override {}
        void onMouseEnter(Swapchain&, bool) override {}
        void onMouseInput(Swapchain&, MouseButton, InputAction, KeyModFlags) override {}
        void onMouseMove(Swapchain&, double, double) override {}
        void onMouseScroll(Swapchain&, double, double) override {}

        void onWindowFocus(Swapchain&, bool) override {}
        void onWindowResize(Swapchain&, uint, uint) override {}
        void onWindowClose(Swapchain&) override {}
        void onWindowMove(Swapchain&, int, int) override {}
        void onWindowRefresh(Swapchain&) override {}
    };
} // namespace trc
