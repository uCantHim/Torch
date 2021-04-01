#pragma once

#include "RenderPass.h"
#include "Pipeline.h"
#include "PipelineBuilder.h"

namespace trc
{
    auto makeGrassPipeline(const vkb::Device& device,
                           vk::RenderPass renderPass,
                           SubPass::ID subPass) -> Pipeline;

    auto getGrassPipeline() -> Pipeline::ID;
}
