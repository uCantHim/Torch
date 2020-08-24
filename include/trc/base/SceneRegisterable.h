#pragma once

#include <vector>

#include "SceneBase.h"

namespace trc
{
    /**
     * @brief Class that groups drawable functions into single objects
     *
     * This class is useful to declare drawable functions once and then
     * never modify them again because added functions can't be removed
     * from the object.
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
         * If the object is currently attach to a scene, the new pipeline will
         * be added to that scene.
         */
        void usePipeline(RenderStage::ID renderStage,
                         SubPass::ID subPass,
                         GraphicsPipeline::ID pipeline,
                         DrawableFunction recordCommandBufferFunction);

        /**
         * @brief Add the object to a scene
         *
         * A SceneRegisterable can only be attached to one scene at any given
         * time. If this method is called while the object is still attached
         * to a scene, the object will be detached from that scene and then
         * attached to the new scene.
         */
        void attachToScene(SceneBase& scene);

        /**
         * @brief Remove the object from the scene it is currently attached to
         *
         * This doesn't take a parameter because the object can only be
         * attached to one scene at a time.
         */
        void removeFromScene();

    private:
        using RecordFuncTuple = std::tuple<RenderStage::ID,
                                           SubPass::ID,
                                           GraphicsPipeline::ID,
                                           DrawableFunction>;

        std::vector<RecordFuncTuple> drawableRecordFuncs;
        std::vector<SceneBase::RegistrationID> registrationIDs;
        SceneBase* currentScene{ nullptr };
    };
} // namespace trc
