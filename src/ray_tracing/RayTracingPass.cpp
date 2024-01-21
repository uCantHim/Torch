#include "trc/ray_tracing/RayTracingPass.h"

#include <cassert>

#include "trc/RayShaders.h"
#include "trc/RasterPlugin.h"
#include "trc/base/Barriers.h"
#include "trc/core/PipelineLayoutBuilder.h"
#include "trc/core/RenderConfiguration.h"



trc::RayTracingPass::RayTracingPass(
    const Instance& instance,
    RenderConfig& renderConfig,
    const rt::TLAS& tlas,
    const FrameSpecific<rt::RayBuffer>* _rayBuffer)
    :
    RenderPass({}, 0),
    instance(instance),
    tlas(tlas),
    descriptorPool(
        instance,
        rt::RayBuffer::Image::NUM_IMAGES * _rayBuffer->getFrameClock().getFrameCount()
    )
{
    assert(_rayBuffer != nullptr);

    setRayBuffer(_rayBuffer);

    // ------------------------------- //
    //    Make reflections pipeline    //
    // ------------------------------- //

    auto reflectPipelineLayout = std::make_unique<PipelineLayout>(
        trc::buildPipelineLayout()
        .addStaticDescriptor(descriptorPool.getDescriptorSetLayout(),
                             descriptorProviders.at(rt::RayBuffer::Image::eReflections))
        .addDescriptor(DescriptorName{RasterPlugin::G_BUFFER_DESCRIPTOR}, true)
        .addDescriptor(DescriptorName{RasterPlugin::ASSET_DESCRIPTOR}, true)
        .addDescriptor(DescriptorName{RasterPlugin::SCENE_DESCRIPTOR}, true)
        .addDescriptor(DescriptorName{RasterPlugin::SHADOW_DESCRIPTOR}, true)
        .addDescriptor(DescriptorName{RasterPlugin::GLOBAL_DATA_DESCRIPTOR}, true)
        .build(instance.getDevice(), renderConfig.getResourceConfig())
    );

    auto [reflectPipeline, reflectShaderBindingTable] =
        trc::rt::buildRayTracingPipeline(instance)
        .addRaygenGroup(rt::shaders::getReflectRaygen())
        .beginTableEntry()
            .addMissGroup(rt::shaders::getBlueMiss())
        .endTableEntry()
        .addTrianglesHitGroup(rt::shaders::getReflectHit(), rt::shaders::getAnyhit())
        .build(kMaxReccursionDepth, *reflectPipelineLayout);

    addRayCall({
        .layout   = std::move(reflectPipelineLayout),
        .pipeline = std::make_unique<Pipeline>(std::move(reflectPipeline)),
        .sbt      = std::make_unique<rt::ShaderBindingTable>(std::move(reflectShaderBindingTable)),
        .raygenTableIndex   = 0,
        .missTableIndex     = 1,
        .hitTableIndex      = 2,
        .callableTableIndex = {},
        .outputImage = trc::rt::RayBuffer::Image::eReflections,
    });
}

void trc::RayTracingPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents /*subpassContents*/,
    FrameRenderState& /*frameState*/)
{
    for (auto& call : rayCalls)
    {
        barrier(cmdBuf,
            vk::ImageMemoryBarrier2(
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                *rayBuffer->get().getImage(call.outputImage),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
            )
        );
        call.pipeline->bind(cmdBuf);
        cmdBuf.traceRaysKHR(
            call.sbt->getEntryAddress(call.raygenTableIndex),
            call.sbt->getEntryAddress(call.missTableIndex),
            call.sbt->getEntryAddress(call.hitTableIndex),
            call.sbt->getEntryAddress(call.callableTableIndex),
            rayBuffer->get().getSize().x, rayBuffer->get().getSize().y, 1,
            instance.getDL()
        );
        barrier(cmdBuf,
            vk::ImageMemoryBarrier2(
                vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                vk::AccessFlagBits2::eShaderStorageWrite,
                vk::PipelineStageFlagBits2::eComputeShader,
                vk::AccessFlagBits2::eShaderStorageRead,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                *rayBuffer->get().getImage(call.outputImage),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
            )
        );
    }
}

void trc::RayTracingPass::end(vk::CommandBuffer)
{
}

void trc::RayTracingPass::setRayBuffer(const FrameSpecific<rt::RayBuffer>* newRayBuffer)
{
    this->rayBuffer = newRayBuffer;
    for (auto img : { rt::RayBuffer::Image::eReflections, })
    {
        auto& set = descriptorSets.emplace_back(
            rayBuffer->getFrameClock(),
            [&](ui32 i) {
                return descriptorPool.allocateDescriptorSet(
                    tlas,
                    rayBuffer->getAt(i).getImageView(img)
                );
            }
        );

        descriptorProviders.emplace_back(std::make_shared<FrameSpecificDescriptorProvider>(set));
    }
}

void trc::RayTracingPass::addRayCall(RayTracingCall call)
{
    rayCalls.emplace_back(std::move(call));
}
