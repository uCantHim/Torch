#pragma once

#include <vector>

#include "Renderpass.h"
#include "Pipeline.h"
#include "BasicScene.h"

class Geometry;
class Material;

/**
 * @brief Class that groups drawable functions into single objects
 */
class SceneRegisterable
{
public:
    /**
     * @brief Declare the use of a pipeline for a specific subpass
     *
     * Record command buffers with a specific function for this particular
     * pipeline.
     *
     * Functions added this way after the object has been attached to a
     * scene will be ignored for that scene. The goal of this mechanic is
     * to make it possible to build a unique drawable and then attach this
     * complete result as an object to a scene. It's not meant for runtime
     * customization of existing objects.
     *
     * If you want to do this anyway, you can detach the object, modify
     * it, and then re-attach it to the scene. Be warned however that this
     * might be a costly operation if you have a lot of functions on the
     * object.
     */
    void usePipeline(SubPass::ID subPass,
                     GraphicsPipeline::ID pipeline,
                     DrawableFunction recordCommandBufferFunction);

    /**
     * @brief Add the object to a scene
     *
     * A SceneRegisterable can only be attached to one scene at any given
     * time. If this method is called while the object is still attached
     * to a scene, the object will be detached from that scene and then
     * attached to the new scene.
     *
     * Hint: This can be used to easily re-attach an object to a scene
     * after its pipeline functions have been modified. Though it's not
     * advised to do so.
     */
    void attachToScene(BasicScene& scene);

    /**
     * @brief Remove the object from the scene it is currently attached to
     *
     * This doesn't take a parameter because the object can only be
     * attached to one scene at a time.
     */
    void removeFromScene();

private:
    using RecordFuncTuple = std::tuple<SubPass::ID,
                                       GraphicsPipeline::ID,
                                       DrawableFunction>;

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
