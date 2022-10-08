#pragma once

#include <vkb/VulkanBase.h>

#include <trc_util/Timer.h>

#include "Types.h"
#include "core/Camera.h"
#include "core/Instance.h"
#include "core/DrawConfiguration.h"
#include "core/RenderTarget.h"
#include "core/Window.h"

#include "Scene.h"
#include "TorchRenderConfig.h"
#include "assets/Assets.h"
#include "drawable/Drawable.h"

namespace trc
{
    /**
     * Reserved for future use, but empty for now.
     */
    struct TorchInitInfo {};

    /**
     * @brief Initialize Torch globally
     *
     * Required to be called once before any other Torch functionality can
     * be used. Calls to `init` before `terminate` has been called return
     * immediately.
     *
     * Call `terminate` to de-initialize Torch.
     *
     * Use `initFull` instead to initialize a complete default configuration
     * of Torch with all required objects and services for rendering.
     */
    void init(const TorchInitInfo& info = {});

    /**
     * @brief Poll system events
     */
    void pollEvents();

    /**
     * @brief Destroy all resources allocated by Torch
     *
     * You should release all of your resources before calling this
     * function.
     */
    void terminate();

    auto makeTorchRenderGraph() -> RenderGraph;

    /**
     * @brief A collection of objects required to render stuff
     *
     * It is certainly possible to create all of these yourself. This is
     * a convenient default implementation.
     */
    struct TorchStack
    {
        TorchStack(const TorchStack&) = delete;
        TorchStack(TorchStack&&) noexcept = delete;
        auto operator=(const TorchStack&) -> TorchStack& = delete;
        auto operator=(TorchStack&&) noexcept -> TorchStack& = delete;

        TorchStack(const InstanceCreateInfo& instanceInfo = {},
                   const WindowCreateInfo& windowInfo = {});
        ~TorchStack();

        auto getDevice() -> vkb::Device&;
        auto getInstance() -> Instance&;
        auto getWindow() -> Window&;
        auto getAssetManager() -> AssetManager&;
        auto getShadowPool() -> ShadowPool&;
        auto getRenderTarget() -> RenderTarget&;
        auto getRenderConfig() -> TorchRenderConfig&;

        /**
         * @brief Create a draw configuration with default values
         *
         * @param Scene& scene The scene which to render with the new
         *                     configuration.
         * @param Camera& camera The camera from which to render the scene.
         *
         * @return DrawConfig
         */
        auto makeDrawConfig(Scene& scene, Camera& camera) -> DrawConfig;

        /**
         * @brief Draw a frame
         *
         * Shortcut for `stack.window->drawFrame()`.
         */
        void drawFrame(const DrawConfig& draw);

    private:
        Instance instance;
        Window window;
        AssetManager assetManager;
        ShadowPool shadowPool;
        RenderTarget swapchainRenderTarget;
        TorchRenderConfig renderConfig;

        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> swapchainRecreateListener;
    };

    /**
     * @brief Create a full default configuration of Torch
     */
    auto initFull(const InstanceCreateInfo& instanceInfo = {},
                  const WindowCreateInfo& windowInfo = {}
                  ) -> u_ptr<TorchStack>;
} // namespace trc
