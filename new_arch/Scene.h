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
        struct ID {
            uint32_t* registrationIndex;
        };

        // Keeps the ID struct's index alive
        std::unique_ptr<uint32_t> indexInRegistrationArray;

        // Entry data
        SubPass::ID subPass;            // Might be able to remove this later on
        GraphicsPipeline::ID pipeline;  // Might be able to remove this later on
        Drawable* drawable;
        std::function<void(vk::CommandBuffer)> recordFunction;
    };

public:
    using RegistrationID = DrawableExecutionRegistration::ID;

    auto createDrawable()
    {
        // TODO: I don't think I use a pool anymore
    }

    // TODO: These two functions are out of date
    auto getPipelines(SubPass::ID subpass) -> const std::vector<GraphicsPipeline::ID>&;
    auto getDrawables(GraphicsPipeline::ID pipeline) -> std::vector<Drawable*>&;

    void foreachDrawable(SubPass::ID subpass, GraphicsPipeline::ID pipeline, void func(Drawable&))
    {
        // TODO
    }

    auto registerDrawableStage(
        SubPass::ID subpass,
        GraphicsPipeline::ID usedPipeline,
        std::function<void(vk::CommandBuffer)> commandBufferRecordingFunction
    ) -> DrawableExecutionRegistration::ID;

    void removeDrawable(Drawable& d)
    {
        assert(d.sceneInfo.owningScene == this);

        const auto& info = d.sceneInfo;
        uint32_t numSubPasses = drawablesperPipeline.getMaxIndex();
        for (uint32_t subPass = 0; subPass < numSubPasses; subPass++)
        {
            const auto& [pipelineIndex, drawableIndex] = info.registeredPipelines[subPass];

            auto& drawableArray = drawablesperPipeline[subPass][pipelineIndex];
            assert(drawableIndex < drawableArray.size());

            // Move last drawable in list to index of removed drawable
            std::swap(drawableArray[drawableIndex], drawableArray.back());
            drawableArray.pop_back();
            // Set new drawable index in reference structure of moved drawable
            drawableArray[drawableIndex].drawable->sceneInfo.registeredPipelines[subPass]
                = { pipelineIndex, drawableIndex };
        }
    }

private:
    template<typename T> using PerSubpass = IndexMap<SubPass::ID, T>;
    template<typename T> using PerPipeline = IndexMap<GraphicsPipeline::ID, T>;

    struct DrawableInfo
    {
        Drawable drawable;
        IndexMap<SubPass::ID, std::pair<GraphicsPipeline::ID, size_t>> indicesInPipelineArray;
    };
    IndexMap<uint32_t, DrawableInfo> drawables;

    /**
     * @brief Add a registration to the registration array
     */
    auto insertRegistration(
        SubPass::ID subpass,
        GraphicsPipeline::ID pipeline,
        DrawableExecutionRegistration reg
    ) -> DrawableExecutionRegistration::ID;

    PerSubpass<PerPipeline<std::vector<DrawableExecutionRegistration>>> drawablesperPipeline;
};
