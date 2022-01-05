#pragma once

namespace trc
{
    class Scene;
    class Camera;
    class RenderConfig;

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
