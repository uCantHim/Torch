#pragma once

#include <string>

#include "trc/base/Swapchain.h"

namespace trc
{
    class Instance;

    struct WindowCreateInfo
    {
        uvec2 size{ 1920, 1080 };
        ivec2 pos{ 0, 0 };
        std::string title{ "" };

        SurfaceCreateInfo surfaceCreateInfo{};
        SwapchainCreateInfo swapchainCreateInfo{};

        s_ptr<InputProcessor> inputProcessor{ nullptr };
    };

    class Window : public Swapchain
    {
    public:
        explicit Window(Instance& instance, WindowCreateInfo info = {});

        auto getInstance() -> Instance&;
        auto getInstance() const -> const Instance&;
        auto getDevice() -> Device&;
        auto getDevice() const -> const Device&;

        auto getSwapchain() -> Swapchain&;
        auto getSwapchain() const -> const Swapchain&;

    private:
        Instance* instance;
    };
} // namespace trc
