#pragma once

#include <vector>
#include <unordered_set>
#include <functional>

#include <trc_util/data/IndexMap.h>

#include "../Types.h"
#include "RenderStage.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "DynamicRenderPassContainer.h"

namespace trc
{
    struct DrawEnvironment
    {
        RenderStage::ID currentRenderStage;
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
                RenderStage::ID stage,
                SubPass::ID sub,
                Pipeline::ID pipeline,
                ui32 i)
                :
                renderStageType(stage),
                subPass(sub),
                pipeline(pipeline),
                indexInRegistrationArray(i)
            {}

            RenderStage::ID renderStageType;
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
        UniqueDrawableRegistrationId(const UniqueDrawableRegistrationId&) = delete;
        auto operator=(const UniqueDrawableRegistrationId&) -> UniqueDrawableRegistrationId& = delete;

        UniqueDrawableRegistrationId() = default;
        UniqueDrawableRegistrationId(UniqueDrawableRegistrationId&&) noexcept;
        UniqueDrawableRegistrationId(DrawableExecutionRegistration::ID id,
                                      SceneBase& scene);
        ~UniqueDrawableRegistrationId();

        auto operator=(UniqueDrawableRegistrationId&&) noexcept -> UniqueDrawableRegistrationId&;

        auto getScene() -> SceneBase*;

    private:
        DrawableExecutionRegistration::ID id;
        SceneBase* scene{ nullptr };
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

    class SceneBase : public DynamicRenderPassContainer
    {
    public:
        using RegistrationID = DrawableExecutionRegistration::ID;
        using UniqueRegistrationID = UniqueDrawableRegistrationId;

        /**
         * @brief Get all pipelines used in a subpass
         */
        auto getPipelines(RenderStage::ID renderStageType, SubPass::ID subPass) const noexcept
            -> const std::vector<Pipeline::ID>&;

        /**
         * @brief Invoke all registered draw functions of a subpass and pipeline
         *
         * We need to pass the pipeline object along with the ID because
         * the pipelines are not globally accessible anymore.
         */
        void invokeDrawFunctions(
            RenderStage::ID stage,
            RenderPass& renderPass,
            SubPass::ID subPass,
            Pipeline::ID pipelineId,
            Pipeline& pipeline,
            vk::CommandBuffer cmdBuf
        ) const;

        /**
         * Don't worry, the RegistrationID is the size of a pointer.
         */
        auto registerDrawFunction(
            RenderStage::ID renderStageType,
            SubPass::ID subpass,
            Pipeline::ID usedPipeline,
            DrawableFunction commandBufferRecordingFunction
        ) -> MaybeUniqueRegistrationId;

        void unregisterDrawFunction(RegistrationID id);
        void unregisterDrawFunction(MaybeUniqueRegistrationId id);

    private:
        template<typename T> using PerRenderStage = data::IndexMap<RenderStage::ID::IndexType, T>;
        template<typename T> using PerSubpass = data::IndexMap<SubPass::ID::IndexType, T>;
        template<typename T> using PerPipeline = data::IndexMap<Pipeline::ID::IndexType, T>;

        /**
         * Sorting the functions this way allows me to group all draw calls with
         * the same pipelines together.
         */
        PerRenderStage<
            PerSubpass<
                PerPipeline<
                    std::vector<DrawableExecutionRegistration>
                >
            >
        > drawableRegistrations;

        // Pipeline storage
        void tryInsertPipeline(RenderStage::ID renderStageType,
                               SubPass::ID subpass,
                               Pipeline::ID pipeline);
        void removePipeline(RenderStage::ID renderStageType,
                            SubPass::ID subpass,
                            Pipeline::ID pipeline);

        PerRenderStage<PerSubpass<std::unordered_set<Pipeline::ID>>> uniquePipelines;
        PerRenderStage<PerSubpass<std::vector<Pipeline::ID>>> uniquePipelinesVector;
    };
} // namespace trc
