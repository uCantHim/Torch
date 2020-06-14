#pragma once

#include <vector>

#include "Renderpass.h"
#include "Pipeline.h"
#include "BasicScene.h"

class Geometry;
class Material;

/**
 * @brief Utiliy for functions that can be registered at a scene
 */
class SceneRegisterable
{
public:
    /**
     * @brief Declare the use of a pipeline for a specific subpass
     *
     * Record command buffers with a specific function for this particular
     * pipeline.
     */
    void usePipeline(SubPass::ID subPass,
                     GraphicsPipeline::ID pipeline,
                     std::function<void(vk::CommandBuffer)> recordCommandBufferFunction);

    void attachToScene(BasicScene& scene);
    void removeFromScene();

private:
    using RecordFuncTuple = std::tuple<SubPass::ID,
                                       GraphicsPipeline::ID,
                                       std::function<void(vk::CommandBuffer)>>;

    std::vector<RecordFuncTuple> drawableRecordFuncs;
    std::vector<BasicScene::RegistrationID> registrationIDs;
    BasicScene* currentScene{ nullptr };
};

/**
 * @brief Purely component-based Drawable class
 */
class DefaultDrawable : SceneRegisterable
{
public:
    auto getGeometry();
    auto getMaterial();

    // Look the transformation up in a global array of matrices
    // I can keep a separate array of links to parents
    auto getTransform();

private:
    Geometry* geo;
    Material* material;
};
