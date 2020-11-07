#pragma once

#include <vkb/event/Event.h>

#include "Types.h"

#include "Renderer.h"
#include "Scene.h"

#include "AssetRegistry.h"
#include "Drawable.h"
#include "Light.h"

namespace trc
{
    struct TorchInitInfo
    {
        RendererCreateInfo rendererInfo;
    };

    /**
     * @brief Initialize all required Torch resources
     */
    extern auto init(const TorchInitInfo& info = {}) -> std::unique_ptr<Renderer>;

    /**
     * @brief Destroy all resources allocated by Torch
     *
     * Does call vkb::terminate for you!
     *
     * You should release all of your resources before calling this
     * function.
     */
    extern void terminate();
} // namespace trc
