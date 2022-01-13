#pragma once

#include <vkb/VulkanBase.h>

#include <trc_util/Timer.h>

#include "Types.h"
#include "core/Instance.h"
#include "core/Window.h"
#include "core/DrawConfiguration.h"
#include "core/RenderTarget.h"

#include "Scene.h"
#include "AssetRegistry.h"
#include "asset_import/AssetUtils.h"
#include "drawable/Drawable.h"
#include "drawable/DrawablePool.h"
#include "Light.h"
#include "DeferredRenderConfig.h"

namespace trc
{
    /**
     * Reserved for future use, but empty for now.
     */
    struct TorchInitInfo {};

    /**
     * @brief Initialize Torch globally
     *
     * Only required to be called once, before any other Torch
     * functionality can be used.
     */
    void init(const TorchInitInfo& info = {});

    /**
     * Torch has a single global vk::Instance as the basis for all Torch
     * instances. Retrieve it with with function.
     *
     * @throws std::runtime_error if the instance has not been intitialized
     *                            with trc::init.
     */
    auto getVulkanInstance() -> vkb::VulkanInstance&;

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
        auto getAssetRegistry() -> AssetRegistry&;
        auto getShadowPool() -> ShadowPool&;
        auto getRenderTarget() -> RenderTarget&;
        auto getRenderConfig() -> DeferredRenderConfig&;

        /**
         * @brief Quickly create a draw configuration with default values
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
        AssetRegistry assetRegistry;
        ShadowPool shadowPool;
        RenderTarget swapchainRenderTarget;
        DeferredRenderConfig renderConfig;

        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> swapchainRecreateListener;
    };

    /**
     * @brief Create a full default configuration of Torch
     */
    auto initFull(const InstanceCreateInfo& instanceInfo = {},
                  const WindowCreateInfo& windowInfo = {}
                  ) -> u_ptr<TorchStack>;

    /**
     * @brief Poll system events
     */
    void pollEvents();

    /**
     * @brief Destroy all resources allocated by Torch
     *
     * Does call vkb::terminate for you!
     *
     * You should release all of your resources before calling this
     * function.
     */
    void terminate();
} // namespace trc
