#include "ComputePass.h"



void trc::ComputePass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    DrawEnvironment env;
    env.currentRenderStageType = nullptr;
    env.currentRenderPass = this;
    env.currentSubPass = 1;

    // Execute static compute executions
    for (const auto& [pipeline, func] : staticExecutions)
    {
        auto& p = Pipeline::at(pipeline);
        p.bind(cmdBuf);
        p.bindDefaultPushConstantValues(cmdBuf);
        p.bindStaticDescriptorSets(cmdBuf);
        env.currentPipeline = &p;

        func(env, cmdBuf);
    }
}

void trc::ComputePass::end(vk::CommandBuffer)
{
}

void trc::ComputePass::addStaticExecution(Pipeline::ID pipeline, DrawableFunction executionFunc)
{
    staticExecutions.emplace_back(pipeline, std::move(executionFunc));
}
