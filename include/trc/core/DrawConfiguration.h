#pragma once

#include "Types.h"
#include "Scene.h"
#include "Camera.h"
#include "RenderConfiguration.h"

namespace trc
{
    /**
     * @brief Configuration for a draw call
     */
    struct DrawConfig
    {
        Scene* scene;
        Camera* camera;

        RenderConfig* renderConfig;
    };
} // namespace trc
