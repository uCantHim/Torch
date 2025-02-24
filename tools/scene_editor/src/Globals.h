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

    template<std::invocable T>
    void openFloatingWindow(T&& windowFunc)
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

        auto window = std::make_unique<Window>(std::forward<T>(windowFunc));
        openFloatingViewport(std::move(window));
    }
} // namespace g
