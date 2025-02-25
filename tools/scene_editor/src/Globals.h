#pragma once

#include <format>

#include "Scene.h"
#include "asset/AssetInventory.h"
#include "gui/ImguiWindow.h"

namespace g
{
    auto assets() -> AssetInventory&;
    auto scene() -> Scene&;
    auto torch() -> trc::TorchStack&;

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
