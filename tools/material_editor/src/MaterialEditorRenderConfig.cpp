#include "MaterialEditorRenderConfig.h"

#include <trc/base/Barriers.h>



MaterialEditorRenderPass::MaterialEditorRenderPass(
    const trc::RenderTarget& target,
    const trc::Device& device,
    const MaterialEditorRenderingInfo& info,
    trc::RenderConfig& config)
    :
    trc::RenderPass(vk::UniqueRenderPass{VK_NULL_HANDLE}, 1),
    renderConfig(&config),
    renderTarget(&target),
    renderArea(target.getSize()),
    renderTargetBarrier(info.renderTargetBarrier),
    finalLayout(info.finalLayout),
    renderer(device, target.getFrameClock())
{
}

void MaterialEditorRenderPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents,
    trc::FrameRenderState&)
{
    if (renderTargetBarrier)
    {
        renderTargetBarrier->setImage(renderTarget->getCurrentImage());
        renderTargetBarrier->setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
        renderTargetBarrier->setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
        renderTargetBarrier->setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite);
        trc::barrier(cmdBuf, *renderTargetBarrier);
    }

    vk::RenderingAttachmentInfo colorAttachment{
        renderTarget->getCurrentImageView(),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ResolveModeFlagBits::eNone, VK_NULL_HANDLE, vk::ImageLayout::eUndefined,  // resolve
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        kClearValue
    };
    const vk::Rect2D area{ { 0, 0 }, { renderArea.x, renderArea.y } };
    vk::RenderingInfo info{
        vk::RenderingFlags{},
        area,
        1,        // layer count
        0x00,     // view mask
        colorAttachment,
        nullptr,  // depth attachment
        nullptr,  // stencil attachment
    };

    cmdBuf.beginRendering(info);

    cmdBuf.setScissor(0, area);
    cmdBuf.setViewport(0, vk::Viewport{
        static_cast<float>(area.offset.x), static_cast<float>(area.offset.y + area.extent.height),
        static_cast<float>(area.extent.width), -static_cast<float>(area.extent.height),
        0.0f, 1.0f
    });

    // Draw the graph
    renderer.draw(cmdBuf, *renderConfig);
}

void MaterialEditorRenderPass::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRendering();

    trc::barrier(cmdBuf, vk::ImageMemoryBarrier2{
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::ImageLayout::eColorAttachmentOptimal, finalLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        renderTarget->getCurrentImage(),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    });
}

auto MaterialEditorRenderPass::getRenderer() -> MaterialGraphRenderer&
{
    return renderer;
}



MaterialEditorRenderConfig::MaterialEditorRenderConfig(
    const trc::RenderTarget& renderTarget,
    const trc::Instance& instance,
    const MaterialEditorRenderingInfo& info)
    :
    trc::RenderConfigImplHelper(instance, trc::RenderGraph{}),
    cameraDesc(renderTarget.getFrameClock(), instance.getDevice()),
    renderPass(renderTarget, instance.getDevice(), info, *this)
{
    addDescriptor(trc::DescriptorName{ kCameraDescriptor }, cameraDesc);
    addRenderPass(
        trc::RenderPassName{ kForwardRenderpass },
        [this]() -> RenderPassDefinition { return { *renderPass, 0 }; }
    );

    getRenderGraph().first(kMainRenderStage);
    getRenderGraph().addPass(kMainRenderStage, renderPass);
}

void MaterialEditorRenderConfig::update(const trc::Camera& camera, const GraphRenderData& data)
{
    cameraDesc.update(camera);
    renderPass.getRenderer().uploadData(data);
}

auto MaterialEditorRenderConfig::getCameraDescriptor() const
    -> const trc::DescriptorProviderInterface&
{
    return cameraDesc;
}
