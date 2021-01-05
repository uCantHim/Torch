#pragma once

#include <vector>
#include <unordered_set>
#include <functional>

#include "data_utils/IndexMap.h"
#include "RenderStage.h"
#include "RenderPass.h"
#include "Pipeline.h"

namespace trc
{
    struct DrawEnvironment
    {
        RenderStageType* currentRenderStageType;
        RenderPass* currentRenderPass;
        SubPass::ID currentSubPass;
        Pipeline* currentPipeline;
    };

    using DrawableFunction = std::function<void(const DrawEnvironment&, vk::CommandBuffer)>;

    class SceneBase;

    /**
     * Registers one drawable with one command buffer recording function
     * at a specific pipeline.
     */
    struct DrawableExecutionRegistration
    {
        struct RegistrationIndex
        {
            RegistrationIndex(
                RenderStageType::ID stage,
                SubPass::ID sub,
                Pipeline::ID pipeline,
                ui32 i)
                :
                renderStageType(stage),
                subPass(sub),
                pipeline(pipeline),
                indexInRegistrationArray(i)
            {}

            RenderStageType::ID renderStageType;
            SubPass::ID subPass;
            Pipeline::ID pipeline;
            ui32 indexInRegistrationArray;
        };

        struct ID
        {
            ID() = default;

        private:
            friend class SceneBase;

            ID(DrawableExecutionRegistration& r)
                : regIndex(r.indexInRegistrationArray.get())
            {}

            RegistrationIndex* regIndex{ nullptr };
        };

        DrawableExecutionRegistration() = default;

        /**
         * @brief Construct a registration
         *
         * Leave the index empty, fill it in SceneBase::insertRegistration().
         */
        DrawableExecutionRegistration(
            std::unique_ptr<RegistrationIndex> indexStruct,
            DrawableFunction func);

        // Allows me to modify pointer of all ID structs remotely
        std::unique_ptr<RegistrationIndex> indexInRegistrationArray;

        // Entry data
        DrawableFunction recordFunction;
    };

    /**
     * @brief A unique wrapper for drawable registrations at a scene
     *
     * Automatically unregisters a referenced drawable registration ID from
     * the scene.
     */
    struct UniqueDrawableRegistrationId
    {
    public:
        UniqueDrawableRegistrationId() = default;
        UniqueDrawableRegistrationId(DrawableExecutionRegistration::ID id,
                                     SceneBase& scene);

    private:
        using Destructor = std::function<void(DrawableExecutionRegistration::ID*)>;
        std::unique_ptr<DrawableExecutionRegistration::ID, Destructor> _id;
    };

    /**
     * @brief Transient wrapper that can construct a unique ID on demand
     *
     * Purely used as an r-value. Can be converted to either a plain
     * registration ID or a unique registration wrapper.
     */
    class MaybeUniqueRegistrationId
    {
    public:
        MaybeUniqueRegistrationId(DrawableExecutionRegistration::ID id, SceneBase& scene)
            : id(id), scene(&scene)
        {}

        inline operator DrawableExecutionRegistration::ID() && {
            return id;
        }

        inline operator UniqueDrawableRegistrationId() && {
            return { id, *scene };
        }

        inline auto makeUnique() && -> UniqueDrawableRegistrationId {
            return { id, *scene };
        }

    private:
        DrawableExecutionRegistration::ID id;
        SceneBase* scene;
    };

    class SceneBase
    {
    public:
        using RegistrationID = DrawableExecutionRegistration::ID;
        using UniqueRegistrationID = UniqueDrawableRegistrationId;

        /**
         * @brief Get all pipelines used in a subpass
         */
        auto getPipelines(RenderStageType::ID renderStageType, SubPass::ID subPass) const noexcept
            -> const std::vector<Pipeline::ID>&;

        /**
         * @brief Invoke all registered draw functions of a subpass and pipeline
         *
         * TODO: Can I remove the renderPass parameter?
         */
        void invokeDrawFunctions(
            RenderStageType::ID stage,
            RenderPass::ID renderPass,
            SubPass::ID subPass,
            Pipeline::ID pipeline,
            vk::CommandBuffer cmdBuf
        ) const;

        /**
         * Don't worry, the RegistrationID is the size of a pointer.
         */
        auto registerDrawFunction(
            RenderStageType::ID renderStageType,
            SubPass::ID subpass,
            Pipeline::ID usedPipeline,
            DrawableFunction commandBufferRecordingFunction
        ) -> MaybeUniqueRegistrationId;

        void unregisterDrawFunction(RegistrationID id);
        void unregisterDrawFunction(MaybeUniqueRegistrationId id);

    private:
        template<typename T> using PerRenderStageType = data::IndexMap<RenderStageType::ID::Type, T>;
        template<typename T> using PerSubpass = data::IndexMap<SubPass::ID::Type, T>;
        template<typename T> using PerPipeline = data::IndexMap<Pipeline::ID::Type, T>;

        /**
         * Sorting the functions this way allows me to group all draw calls with
         * the same pipelines together.
         *
         * TODO: Store the functions in a separate array with ONLY functions.
         * Just reference those functions through the indexInRegistrationArray
         * property in DrawableExecutionRegistration.
         */
        PerRenderStageType<
            PerSubpass<
                PerPipeline<
                    std::vector<DrawableExecutionRegistration>
                >
            >
        > drawableRegistrations;

        // Pipeline storage
        void tryInsertPipeline(RenderStageType::ID renderStageType,
                               SubPass::ID subpass,
                               Pipeline::ID pipeline);
        void removePipeline(RenderStageType::ID renderStageType,
                            SubPass::ID subpass,
                            Pipeline::ID pipeline);

        PerRenderStageType<PerSubpass<std::unordered_set<Pipeline::ID>>> uniquePipelines;
        PerRenderStageType<PerSubpass<std::vector<Pipeline::ID>>> uniquePipelinesVector;
    };
} // namespace trc
