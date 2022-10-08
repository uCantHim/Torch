#pragma once

#include <string>

#include "trc/base/Swapchain.h"
#include "trc/base/event/Event.h"

#include "trc/core/Instance.h"
#include "trc/core/Renderer.h"

namespace trc
{
    struct WindowCreateInfo
    {
        uvec2 size{ 1920, 1080 };
        ivec2 pos{ 0, 0 };
        std::string title{ "" };

        SurfaceCreateInfo surfaceCreateInfo{};
        SwapchainCreateInfo swapchainCreateInfo{};
    };

    /**
     * @brief
     */
    class Window : public Swapchain
    {
    public:
        /**
         * @brief
         */
        explicit Window(Instance& instance, WindowCreateInfo info = {});

        void drawFrame(const vk::ArrayProxy<const DrawConfig>& draws);

        auto getInstance() -> Instance&;
        auto getInstance() const -> const Instance&;
        auto getDevice() -> Device&;
        auto getDevice() const -> const Device&;

        auto getSwapchain() -> Swapchain&;
        auto getSwapchain() const -> const Swapchain&;
        auto getRenderer() -> Renderer&;

    private:
        Instance* instance;

        u_ptr<Renderer> renderer;
        UniqueListenerId<PreSwapchainRecreateEvent> preRecreateListener;
        UniqueListenerId<SwapchainRecreateEvent> recreateListener;
    };
} // namespace trc
