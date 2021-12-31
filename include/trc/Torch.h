#pragma once

#include <vkb/VulkanBase.h>

#include <trc_util/Timer.h>

#include "Types.h"
#include "core/Instance.h"
#include "core/Window.h"
#include "core/DrawConfiguration.h"

#include "Scene.h"
#include "AssetRegistry.h"
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

    /**
     * @brief A collection of objects required to render stuff
     *
     * It is certainly possible to create all of these yourself. This is
     * a convenient default implementation.
     */
    struct TorchStack
    {
        u_ptr<Instance> instance;
        u_ptr<Window> window;
        u_ptr<AssetRegistry> assetRegistry;
        u_ptr<ShadowPool> shadowPool;
        u_ptr<DeferredRenderConfig> renderConfig;

        TorchStack() = default;
        TorchStack(TorchStack&&) noexcept = default;
        auto operator=(TorchStack&&) noexcept -> TorchStack& = default;

        TorchStack(const TorchStack&) = delete;
        auto operator=(const TorchStack&) -> TorchStack& = delete;

        TorchStack(u_ptr<Instance> instance,
                   u_ptr<Window> window,
                   u_ptr<AssetRegistry> assetRegistry,
                   u_ptr<ShadowPool> shadowPool,
                   u_ptr<DeferredRenderConfig> renderConfig);
        ~TorchStack();

        /**
         * @brief Quickly create a draw configuration with default values
         *
         * @param Scene& scene The scene which to render with the new
         *                     configuration.
         * @param Camera& camera The camera from which to render the scene.
         *
         * @return DrawConfig
         */
        auto makeDrawConfig(Scene& scene, Camera& camera) const -> DrawConfig;

        /**
         * @brief Draw a frame
         *
         * Shortcut for `stack.window->drawFrame()`.
         */
        void drawFrame(const DrawConfig& draw);
    };

    /**
     * @brief Create a full default configuration of Torch
     */
    auto initFull(const InstanceCreateInfo& instanceInfo = {},
                  const WindowCreateInfo& windowInfo = {}
                  ) -> TorchStack;

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
