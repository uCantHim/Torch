#pragma once

#include <vector>
#include <set>

#include "IndexMap.h"
#include "Renderpass.h"
#include "Pipeline.h"
#include "Drawable.h"

class Scene
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
        private:
            friend Scene;

            ID(DrawableExecutionRegistration** r) : reg(r) {}

            DrawableExecutionRegistration** reg;
        };

        DrawableExecutionRegistration() = default;

        /**
         * @brief Construct a registration
         *
         * Leave the index empty, fill it in Scene::insertRegistration().
         */
        DrawableExecutionRegistration(
            SubPass::ID s,
            GraphicsPipeline::ID p,
            std::function<void(vk::CommandBuffer)> func);

        // Allows me to modify all ID struct's pointer remotely
        std::unique_ptr<DrawableExecutionRegistration*> thisPointer{ nullptr };
        uint32_t indexInRegistrationArray;

        // Entry data
        SubPass::ID subPass;            // Might be able to remove this later on
        GraphicsPipeline::ID pipeline;  // Might be able to remove this later on
        std::function<void(vk::CommandBuffer)> recordFunction;
    };

public:
    using RegistrationID = DrawableExecutionRegistration::ID;

    /**
     * @brief Get all pipelines used in a subpass
     */
    auto getPipelines(SubPass::ID subpass) const noexcept
        -> const std::vector<GraphicsPipeline::ID>&;

    auto getDrawFunctions(SubPass::ID subpass, GraphicsPipeline::ID pipeline) const noexcept
        -> const std::vector<std::function<void(vk::CommandBuffer)>>&;

    void invokeDrawFunctions(
        SubPass::ID subpass,
        GraphicsPipeline::ID pipeline,
        vk::CommandBuffer cmdBuf
    ) const;

    auto registerDrawFunction(
        SubPass::ID subpass,
        GraphicsPipeline::ID usedPipeline,
        std::function<void(vk::CommandBuffer)> commandBufferRecordingFunction
    ) -> RegistrationID;

    void unregisterDrawFunction(RegistrationID id);

private:
    template<typename T> using PerSubpass = IndexMap<SubPass::ID, T>;
    template<typename T> using PerPipeline = IndexMap<GraphicsPipeline::ID, T>;

    /**
     * @brief Add a registration to the registration array
     */
    auto insertRegistration(DrawableExecutionRegistration reg) -> RegistrationID;

    // TODO: Store the functions in a separate array with ONLY functions.
    // Just reference those functions through the indexInRegistrationArray
    // property in DrawableExecutionRegistration.
    PerSubpass<PerPipeline<std::vector<DrawableExecutionRegistration>>> drawableRegistrations;

    // Pipeline storage
    void tryInsertPipeline(SubPass::ID subpass, GraphicsPipeline::ID pipeline);
    void removePipeline(SubPass::ID subpass, GraphicsPipeline::ID pipeline);

    PerSubpass<std::set<GraphicsPipeline::ID>> uniquePipelines;
    PerSubpass<std::vector<GraphicsPipeline::ID>> uniquePipelinesVector;
};
