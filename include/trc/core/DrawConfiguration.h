#pragma once

namespace trc
{
    class RasterSceneBase;
    class RenderConfig;

    /**
     * @brief Configuration for a draw call
     */
    struct DrawConfig
    {
        const RasterSceneBase& scene;
        RenderConfig& renderConfig;
    };
} // namespace trc
