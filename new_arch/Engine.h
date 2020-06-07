#pragma once

#include "Renderpass.h"
#include "Scene.h"

class Engine
{
public:
    /**
     * Do some cool things here
     */
    void drawScene(RenderPass& renderpass, Scene& scene)
    {
        for (auto subpass : renderpass.getSubPasses())
        {
            for (auto pipeline : scene.getPipelines(subpass))
            {
                // Collect commands for all drawables with this
                // particular pipeline.
                for (auto drawable : scene.getDrawables(pipeline))
                {
                    // Record to primary command buffer
                }
            }
        }
    }
};
