#include "trc/ray_tracing/RayTracingPass.h"

#include "trc/base/Barriers.h"

#include "trc/TorchRenderConfig.h"
#include "trc/core/PipelineLayoutBuilder.h"



trc::RayTracingPass::RayTracingPass(
    const Instance& instance,
    TorchRenderConfig& renderConfig,
    const rt::TLAS& tlas,
    FrameSpecific<rt::RayBuffer> _rayBuffer,
    const RenderTarget& target)
    :
    RenderPass({}, 0),
    instance(instance),
    tlas(tlas),
    rayBuffer(std::move(_rayBuffer)),
    descriptorPool(
        instance,
        rt::RayBuffer::Image::NUM_IMAGES * rayBuffer.getFrameClock().getFrameCount()
    ),
    compositingPass(instance, target, rayBuffer)
{
    // ------------------------------- //
    //    Make reflections pipeline    //
    // ------------------------------- //

    auto& tlasDescProvider = makeDescriptor(rt::RayBuffer::Image::eReflections);
    auto reflectPipelineLayout = std::make_unique<PipelineLayout>(
        trc::buildPipelineLayout()
        .addDescriptor(tlasDescProvider, true)
        .addDescriptor(renderConfig.getGBufferDescriptorProvider(), true)
        .addDescriptor(renderConfig.getAssets().getDescriptorSetProvider(), true)
        .addDescriptor(renderConfig.getSceneDescriptorProvider(), true)
        .addDescriptor(renderConfig.getShadowDescriptorProvider(), true)
        .addDescriptor(renderConfig.getGlobalDataDescriptorProvider(), true)
        .build(instance.getDevice(), renderConfig)
    );

    auto [reflectPipeline, reflectShaderBindingTable] =
        trc::rt::buildRayTracingPipeline(instance)
        .addRaygenGroup("/ray_tracing/reflect.rgen")
        .beginTableEntry()
            .addMissGroup("/ray_tracing/blue.rmiss")
        .endTableEntry()
        .addTrianglesHitGroup("/ray_tracing/reflect.rchit", "/ray_tracing/anyhit.rahit")
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
    vk::SubpassContents subpassContents,
    FrameRenderState& frameState)
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
                *rayBuffer->getImage(call.outputImage),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
            )
        );
        call.pipeline->bind(cmdBuf);
        cmdBuf.traceRaysKHR(
            call.sbt->getEntryAddress(call.raygenTableIndex),
            call.sbt->getEntryAddress(call.missTableIndex),
            call.sbt->getEntryAddress(call.hitTableIndex),
            call.sbt->getEntryAddress(call.callableTableIndex),
            rayBuffer->getSize().x, rayBuffer->getSize().y, 1,
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
                *rayBuffer->getImage(call.outputImage),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
            )
        );
    }

    compositingPass.begin(cmdBuf, subpassContents, frameState);
}

void trc::RayTracingPass::end(vk::CommandBuffer cmdBuf)
{
    compositingPass.end(cmdBuf);
}

void trc::RayTracingPass::setRenderTarget(const RenderTarget& target)
{
    compositingPass.setRenderTarget(target);
}

void trc::RayTracingPass::addRayCall(RayTracingCall call)
{
    rayCalls.emplace_back(std::move(call));
}

auto trc::RayTracingPass::makeDescriptor(rt::RayBuffer::Image rayBufferImage)
    -> const DescriptorProviderInterface&
{
    auto& set = descriptorSets.emplace_back(
        rayBuffer.getFrameClock(),
        [&](ui32 i) {
            return descriptorPool.allocateDescriptorSet(
                tlas,
                rayBuffer.getAt(i).getImageView(rayBufferImage)
            );
        }
    );

    return *descriptorProviders.emplace_back(
        std::make_shared<trc::FrameSpecificDescriptorProvider>(
            descriptorPool.getDescriptorSetLayout(),
            set
        )
    );
}
