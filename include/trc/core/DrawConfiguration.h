#pragma once

#include "Types.h"
#include "Scene.h"
#include "Camera.h"
#include "RenderConfiguration.h"

namespace trc
{
    struct RenderArea
    {
        vk::Viewport viewport;
        vk::Rect2D scissor;
    };

    /**
     * @brief Configuration for a draw call
     */
    struct DrawConfig
    {
        Scene* scene;
        Camera* camera;

        RenderConfig* renderConfig;
        RenderArea renderArea;
    };
} // namespace trc
