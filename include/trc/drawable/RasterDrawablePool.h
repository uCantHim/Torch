#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>

#include "core/RenderStage.h"
#include "core/Pipeline.h"
#include "core/SceneBase.h"

#include "DrawablePoolStructs.h"

namespace trc
{
    class SceneBase;

    struct RasterDrawablePoolCreateInfo
    {
        ui32 maxDrawables{ 1000 };
    };

    /**
     * @brief
     */
    class RasterDrawablePool
    {
    public:
        explicit RasterDrawablePool(const RasterDrawablePoolCreateInfo& info);

        void attachToScene(SceneBase& scene);

        void createDrawable(ui32 drawableId, const DrawableCreateInfo& info);
        void deleteDrawable(ui32 drawableId);
        void createInstance(ui32 drawableId, const DrawableInstanceCreateInfo& info);
        void deleteInstance(ui32 drawableId, ui32 instanceId);

    private:
        struct Instance
        {
            Transformation::ID transform;
            AnimationEngine::ID animData;
        };

        /**
         * Minimal usable data for one drawable. Instances are born out of
         * this.
         */
        struct DrawableData
        {
            Geometry geo;
            MaterialID material;

            u_ptr<std::mutex> instancesLock{ new std::mutex };
            std::vector<Instance> instances;  // Instances at fixed indices
        };

        struct DrawableMeta
        {
            std::vector<Pipeline::ID> pipelines;  // Pipelines to which the drawable is attached
        };

        auto getRequiredPipelines(const DrawableCreateInfo& info) -> std::vector<Pipeline::ID>;
        void addToPipeline(Pipeline::ID pipeline, ui32 drawableId);
        void removeFromPipeline(Pipeline::ID pipeline, ui32 drawableId);

        std::vector<DrawableData> drawables;  // Draw data at fixed indices
        std::vector<DrawableMeta> drawableMetas;


        std::unordered_map<Pipeline::ID, std::pair<std::vector<ui32>, std::mutex>> drawCalls;


        ///////////////////////////
        // Draw-execution specifics

        void createRasterFunctions();

        struct DrawFunctionSpec
        {
            DrawableFunction func;
            SubPass::ID subpass;
            RenderStage::ID stage;
        };

        std::unordered_map<Pipeline::ID, DrawFunctionSpec> drawFunctions;
        std::vector<SceneBase::UniqueRegistrationID> drawRegistrations;
    };
} // namespace trc
