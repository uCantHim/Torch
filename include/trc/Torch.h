#pragma once

#include <vkb/event/Event.h>

#include "Types.h"
#include "Instance.h"
#include "Window.h"

#include "Scene.h"
#include "AssetRegistry.h"
#include "drawable/Drawable.h"
#include "Light.h"
#include "DeferredRenderConfig.h"

namespace trc
{
    struct TorchInitInfo
    {
    };

    /**
     * @brief Initialize Torch globally
     *
     * Only required to be called once, before any other Torch
     * functionality can be used.
     */
    void init(const TorchInitInfo& info = {});

    auto getVulkanInstance() -> vkb::VulkanInstance&;

    /**
     * @brief A collection of objects necessary for rendering with Torch
     */
    struct DefaultTorchStack
    {
        u_ptr<Instance> instance;
        u_ptr<Window> window;
        u_ptr<DeferredRenderConfig> renderConfig;
    };

    /**
     * @brief Create a default configuration of Torch
     *
     * This is fine in most cases.
     */
    auto initDefault() -> DefaultTorchStack;

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
