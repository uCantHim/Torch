#pragma once

namespace trc
{
    class SceneBase;
    class RenderConfig;

    /**
     * @brief Configuration for a draw call
     */
    struct DrawConfig
    {
        const SceneBase& scene;
        RenderConfig& renderConfig;
    };
} // namespace trc
