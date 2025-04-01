#pragma once

#include <format>

#include "Scene.h"
#include "asset/AssetInventory.h"
#include "gui/ImguiWindow.h"
#include "input/KeyboardState.h"
#include "input/MouseState.h"

namespace g
{
    /** Access the global keyboard state */
    auto keyboard() -> const KeyboardState&;

    /** Access the global mouse state */
    auto mouse() -> const MouseState&;

    auto assets() -> AssetInventory&;
    auto scene() -> Scene&;

    void openFloatingViewport(s_ptr<Viewport> vp);
    void closeFloatingViewport(Viewport* vp);

    template<std::invocable T>
    [[nodiscard]]
    auto openFloatingWindow(T&& windowFunc) -> s_ptr<Viewport>
    {
        struct Window : public ImguiWindow
        {
            Window(T&& func)
                : ImguiWindow(std::format("##{}", (uintptr_t)this), ImguiWindowType::eFloating)
                , func(std::move(func))
            {}

            void drawWindowContent() override {
                func();
            }

            T func;
        };

        auto window = std::make_shared<Window>(std::forward<T>(windowFunc));
        openFloatingViewport(window);
        return window;
    }
} // namespace g
