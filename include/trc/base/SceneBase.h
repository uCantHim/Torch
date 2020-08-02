#pragma once

#include <vector>
#include <set>

#include "data_utils/IndexMap.h"
#include "RenderPass.h"
#include "Pipeline.h"

namespace trc
{
    using DrawableFunction = std::function<void(vk::CommandBuffer)>;

    class SceneBase
    {
    private:
        /**
         * Registers one drawable with one command buffer recording function
         * at a specific pipeline.
         */
        struct DrawableExecutionRegistration
        {
            struct ID
            {
                ID() = default;

            private:
                friend SceneBase;

                ID(DrawableExecutionRegistration** r) : reg(r) {}

                DrawableExecutionRegistration** reg{ nullptr };
            };

            DrawableExecutionRegistration() = default;

            /**
             * @brief Construct a registration
             *
             * Leave the index empty, fill it in SceneBase::insertRegistration().
             */
            DrawableExecutionRegistration(
                RenderPass::ID r,
                SubPass::ID s,
                GraphicsPipeline::ID p,
                DrawableFunction func);

            // Allows me to modify pointer of all ID structs remotely
            std::unique_ptr<DrawableExecutionRegistration*> thisPointer{ nullptr };
            ui32 indexInRegistrationArray;

            // Entry data
            RenderPass::ID renderPass;
            SubPass::ID subPass;
            GraphicsPipeline::ID pipeline;

            DrawableFunction recordFunction;
        };

    public:
        using RegistrationID = DrawableExecutionRegistration::ID;

        /**
         * @brief Get all pipelines used in a subpass
         */
        auto getPipelines(RenderPass::ID renderPass, SubPass::ID subpass) const noexcept
            -> const std::set<GraphicsPipeline::ID>&;

        /**
         * @brief Invoke all registered draw functions of a subpass and pipeline
         */
        void invokeDrawFunctions(
            RenderPass::ID renderPass,
            SubPass::ID subpass,
            GraphicsPipeline::ID pipeline,
            vk::CommandBuffer cmdBuf
        ) const;

        /**
         * Don't worry, the RegistrationID is the size of a pointer.
         */
        auto registerDrawFunction(
            RenderPass::ID renderPass,
            SubPass::ID subpass,
            GraphicsPipeline::ID usedPipeline,
            DrawableFunction commandBufferRecordingFunction
        ) -> RegistrationID;

        void unregisterDrawFunction(RegistrationID id);

    private:
        template<typename T> using PerRenderPass = data::IndexMap<RenderPass::ID, T>;
        template<typename T> using PerSubpass = data::IndexMap<SubPass::ID, T>;
        template<typename T> using PerPipeline = data::IndexMap<GraphicsPipeline::ID, T>;

        /**
         * @brief Add a registration to the registration array
         */
        auto insertRegistration(DrawableExecutionRegistration reg) -> RegistrationID;

        /**
         * Sorting the functions this way allows me to group all draw calls with
         * the same pipelines together.
         *
         * TODO: Store the functions in a separate array with ONLY functions.
         * Just reference those functions through the indexInRegistrationArray
         * property in DrawableExecutionRegistration.
         */
        PerRenderPass<
            PerSubpass<
                PerPipeline<
                    std::vector<DrawableExecutionRegistration>
                >
            >
        > drawableRegistrations;

        // Pipeline storage
        void tryInsertPipeline(RenderPass::ID renderPass,
                               SubPass::ID subpass,
                               GraphicsPipeline::ID pipeline);
        void removePipeline(RenderPass::ID renderPass,
                            SubPass::ID subpass,
                            GraphicsPipeline::ID pipeline);

        PerRenderPass<PerSubpass<std::set<GraphicsPipeline::ID>>> uniquePipelines;
        PerRenderPass<PerSubpass<std::vector<GraphicsPipeline::ID>>> uniquePipelinesVector;
    };
} // namespace trc
