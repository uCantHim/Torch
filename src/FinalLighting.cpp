#include "trc/FinalLighting.h"

#include <vector>

#include "trc/PipelineDefinitions.h"
#include "trc/RasterPipelines.h"
#include "trc/RasterPlugin.h"
#include "trc/TorchRenderStages.h"
#include "trc/core/ComputePipelineBuilder.h"
#include "trc/core/Frame.h"
#include "trc/core/PipelineLayoutBuilder.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/DeviceTask.h"



trc::FinalLightingDispatcher::FinalLightingDispatcher(
    const Viewport& viewport,
    Pipeline::ID pipeline,
    vk::UniqueDescriptorSet renderTargetDescSet)
    :
    groupCount{ (uvec3(viewport.area.size, 1) + kLocalGroupSize - 1u) / kLocalGroupSize },
    renderOffset(viewport.area.offset),
    renderSize(viewport.area.size),
    targetImage(viewport.target.image),
    pipeline(pipeline),
    descSet(std::move(renderTargetDescSet))
{
}

void trc::FinalLightingDispatcher::createTasks(ViewportDrawTaskQueue& queue)
{
    queue.spawnTask(
        stages::deferredLighting,
        [this](vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx){
            const auto& pipeline = ctx.resources().getPipeline(this->pipeline);
            const auto layout = *pipeline.getLayout();

            pipeline.bind(cmdBuf, ctx.resources());
            cmdBuf.pushConstants<vec2>(layout,
                                       vk::ShaderStageFlagBits::eCompute,
                                       0, { renderOffset, renderSize });
            cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                      layout,
                                      2,  // set index
                                      *descSet, {});

            ctx.deps().consume(ImageAccess{
                targetImage,
                vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::ImageLayout::eGeneral,
            });

            cmdBuf.dispatch(groupCount.x, groupCount.y, groupCount.z);

            ctx.deps().produce(ImageAccess{
                targetImage,
                vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderWrite,
                vk::ImageLayout::eGeneral,
            });
        }
    );
}

trc::FinalLighting::FinalLighting(
    const Device& device,
    const ui32 maxInstances)
{
    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageImage, 1 },
    };
    descPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        maxInstances,
        poolSizes
    });

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute }
    };
    descLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo(
        {}, // vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR
        layoutBindings
    ));

    device.setDebugName(*descPool, "Final lighting output image descriptor pool");
    device.setDebugName(*descLayout, "Final lighting output image descriptor layout");

    // Compute pipeline
    layout = buildPipelineLayout()
        .addDescriptor(DescriptorName{ RasterPlugin::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ OUTPUT_IMAGE_DESCRIPTOR }, false)
        .addDescriptor(DescriptorName{ RasterPlugin::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::SHADOW_DESCRIPTOR }, true)
        .addPushConstantRange({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(vec2) * 2 })
        .registerLayout();
    pipeline = buildComputePipeline()
        .setProgram(internal::loadShader(pipelines::getFinalLightingShader()))
        .registerPipeline(layout);
}

auto trc::FinalLighting::makeDrawConfig(const Device& device, const Viewport& viewport)
    -> u_ptr<FinalLightingDispatcher>
{
    // Create the descriptor set
    auto descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, *descLayout })[0]);
    device.setDebugName(*descSet, "Final lighting output image descriptor set");

    vk::DescriptorImageInfo imageInfo({}, viewport.target.imageView, vk::ImageLayout::eGeneral);
    vk::WriteDescriptorSet write(
        *descSet, 0, 0, vk::DescriptorType::eStorageImage, imageInfo
    );
    device->updateDescriptorSets(write, {});

    return std::make_unique<FinalLightingDispatcher>(
        viewport,
        pipeline,
        std::move(descSet)
    );
}

auto trc::FinalLighting::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *descLayout;
}
