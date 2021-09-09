#pragma once

#include <string>

#include <vkb/basics/Swapchain.h>

#include "Instance.h"
#include "Renderer.h"

namespace trc
{
    struct WindowCreateInfo
    {
        uvec2 size{ 1920, 1080 };
        std::string title = "";

        vkb::SwapchainCreateInfo swapchainCreateInfo{};
    };

    /**
     * @brief
     */
    class Window
    {
    public:
        /**
         * @brief
         */
        explicit Window(Instance& instance, WindowCreateInfo info = {});

        void drawFrame(const DrawConfig& drawConfig);

        auto getInstance() -> Instance&;
        auto getInstance() const -> const Instance&;
        auto getDevice() -> vkb::Device&;
        auto getDevice() const -> const vkb::Device&;

        auto getSwapchain() -> vkb::Swapchain&;
        auto getSwapchain() const -> const vkb::Swapchain&;
        auto getRenderer() -> Renderer&;

        auto makeFullscreenRenderArea() const -> RenderArea;

    private:
        Instance* instance;

        vkb::Swapchain swapchain;
        Renderer renderer;
    };
} // namespace trc
