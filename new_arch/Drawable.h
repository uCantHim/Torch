#pragma once

#include <vector>

#include "Renderpass.h"
#include "Pipeline.h"

class Scene;
class Geometry;
class Material;

class DrawableType
{
public:
    virtual auto getCommandBuffer(SubPass::ID subpass) -> vk::CommandBuffer = 0;
};

/**
 * @brief Purely component-based Drawable class
 */
class Drawable
{
public:
    /**
     * @brief Declare the use of a pipeline for a specific subpass
     *
     * Record command buffers with a specific function for this particular
     * pipeline.
     */
    void usePipeline(SubPass::ID pass,
                     GraphicsPipeline::ID pipeline,
                     std::function<void(vk::CommandBuffer)> recordCommandBufferFunction);
    {
        owningScene->
    }

    auto getGeometry();
    auto getMaterial();

    // Look the transformation up in a global array of matrices
    // I can keep a separate array of links to parents
    auto getTransform();

private:
    friend class Scene;

    struct SceneInfo
    {
        // Notify on pipeline change
        Scene* owningScene;
        uint32_t index;
        // Indices in vectors of specific pipelines
        // For fast removal of drawables from the per-pipeline vectors
        IndexMap<SubPass::ID, std::pair<GraphicsPipeline::ID, uint32_t>> registeredPipelines;
    };
    SceneInfo sceneInfo;

    std::unique_ptr<DrawableType> type;
    Geometry* geo;
    Material* material;

uint32_t commandBufferRecorder;
};
