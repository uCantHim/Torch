#pragma once

#include <vector>

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
     * @brief Configuration for a single draw call
     */
    struct DrawConfig
    {
        Scene* scene;
        Camera* camera;

        RenderConfig* renderConfig;
        std::vector<RenderArea> renderAreas;
    };
} // namespace trc
