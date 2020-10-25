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
        RenderStage* currentRenderStage;
        RenderPass* currentRenderPass;
        SubPass::ID currentSubPass;
        Pipeline* currentPipeline;
    };

    using DrawableFunction = std::function<void(const DrawEnvironment&, vk::CommandBuffer)>;

    class SceneBase
    {
    private:
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
                    renderStage(stage), subPass(sub), pipeline(pipeline), indexInRegistrationArray(i) {}

                RenderStage::ID renderStage;
                SubPass::ID subPass;
                Pipeline::ID pipeline;
                ui32 indexInRegistrationArray;
            };

            struct ID
            {
                ID() = default;

            private:
                friend SceneBase;

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

    public:
        using RegistrationID = DrawableExecutionRegistration::ID;

        /**
         * @brief Get all pipelines used in a subpass
         */
        auto getPipelines(RenderStage::ID renderStage, SubPass::ID subPass) const noexcept
            -> const std::unordered_set<Pipeline::ID>&;

        /**
         * @brief Invoke all registered draw functions of a subpass and pipeline
         *
         * TODO: Can I remove the renderPass parameter?
         */
        void invokeDrawFunctions(
            RenderStage::ID stage,
            RenderPass::ID renderPass,
            SubPass::ID subPass,
            Pipeline::ID pipeline,
            vk::CommandBuffer cmdBuf
        ) const;

        /**
         * Don't worry, the RegistrationID is the size of a pointer.
         */
        auto registerDrawFunction(
            RenderStage::ID renderStage,
            SubPass::ID subpass,
            Pipeline::ID usedPipeline,
            DrawableFunction commandBufferRecordingFunction
        ) -> RegistrationID;

        void unregisterDrawFunction(RegistrationID id);

    private:
        template<typename T> using PerRenderStage = data::IndexMap<RenderStage::ID::Type, T>;
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
        PerRenderStage<
            PerSubpass<
                PerPipeline<
                    std::vector<DrawableExecutionRegistration>
                >
            >
        > drawableRegistrations;

        // Pipeline storage
        void tryInsertPipeline(RenderStage::ID renderStage,
                               SubPass::ID subpass,
                               Pipeline::ID pipeline);
        void removePipeline(RenderStage::ID renderStage,
                            SubPass::ID subpass,
                            Pipeline::ID pipeline);

        PerRenderStage<PerSubpass<std::unordered_set<Pipeline::ID>>> uniquePipelines;
        PerRenderStage<PerSubpass<std::vector<Pipeline::ID>>> uniquePipelinesVector;
    };
} // namespace trc
