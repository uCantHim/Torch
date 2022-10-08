#pragma once

#include <string>

#include <vkb/Swapchain.h>
#include <vkb/event/Event.h>

#include "Instance.h"
#include "Renderer.h"

namespace trc
{
    struct WindowCreateInfo
    {
        uvec2 size{ 1920, 1080 };
        ivec2 pos{ 0, 0 };
        std::string title{ "" };

        vkb::SurfaceCreateInfo surfaceCreateInfo{};
        vkb::SwapchainCreateInfo swapchainCreateInfo{};
    };

    /**
     * @brief
     */
    class Window : public vkb::Swapchain
    {
    public:
        /**
         * @brief
         */
        explicit Window(Instance& instance, WindowCreateInfo info = {});

        void drawFrame(const vk::ArrayProxy<const DrawConfig>& draws);

        auto getInstance() -> Instance&;
        auto getInstance() const -> const Instance&;
        auto getDevice() -> vkb::Device&;
        auto getDevice() const -> const vkb::Device&;

        auto getSwapchain() -> vkb::Swapchain&;
        auto getSwapchain() const -> const vkb::Swapchain&;
        auto getRenderer() -> Renderer&;

    private:
        Instance* instance;

        u_ptr<Renderer> renderer;
        vkb::UniqueListenerId<vkb::PreSwapchainRecreateEvent> preRecreateListener;
        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> recreateListener;
    };
} // namespace trc
