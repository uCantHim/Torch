#include "MaterialEditorRenderConfig.h"

#include <trc/TorchRenderStages.h>
#include <trc/base/Barriers.h>



void MaterialEditorRenderPass::begin(
    vk::CommandBuffer cmdBuf,
    trc::ViewportDrawContext& ctx)
{
    ctx.deps().consume(trc::ImageAccess{
        ctx.renderImage().image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
    });

    const vk::Rect2D area{
        { ctx.renderArea().offset.x, ctx.renderArea().offset.y, },
        { ctx.renderArea().size.x, ctx.renderArea().size.y, },
    };
    vk::RenderingAttachmentInfo colorAttachment{
        ctx.renderImage().imageView,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ResolveModeFlagBits::eNone, VK_NULL_HANDLE, vk::ImageLayout::eUndefined,  // resolve
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        kClearValue
    };
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
        static_cast<float>(area.offset.x), static_cast<float>(area.offset.y),
        static_cast<float>(area.extent.width), static_cast<float>(area.extent.height),
        0.0f, 1.0f
    });
}

void MaterialEditorRenderPass::end(vk::CommandBuffer cmdBuf, trc::ViewportDrawContext& ctx)
{
    cmdBuf.endRendering();

    ctx.deps().produce(trc::ImageAccess{
        ctx.renderImage().image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
    });
}



MaterialEditorRenderPlugin::MaterialEditorRenderPlugin(
    const trc::Device& device,
    const trc::RenderTarget& renderTarget)
    :
    renderTargetFormat(renderTarget.getFormat()),
    cameraDesc(std::make_shared<CameraDescriptor>(renderTarget.getFrameClock(), device)),
    renderer(device, renderTarget.getFrameClock())
{
}

void MaterialEditorRenderPlugin::defineRenderStages(trc::RenderGraph& graph)
{
    graph.createOrdering(kMainRenderStage, trc::imgui::imguiRenderStage);
    graph.createOrdering(trc::stages::renderTargetImageInit, kMainRenderStage);
    graph.createOrdering(kMainRenderStage, trc::stages::renderTargetImageFinalize);
}

void MaterialEditorRenderPlugin::defineResources(trc::ResourceConfig& config)
{
    config.defineDescriptor(trc::DescriptorName{ kCameraDescriptor },
                            cameraDesc->getDescriptorSetLayout());
    config.defineDescriptor(trc::DescriptorName{ kTextureDescriptor },
                            renderer.getTextureDescriptorLayout());
    config.addRenderPass(kForwardRenderpass, trc::DynamicRenderingInfo{
        .viewMask=0x00,
        .colorAttachmentFormats{ renderTargetFormat },
        .depthAttachmentFormat{},
        .stencilAttachmentFormat{},
    });
}

auto MaterialEditorRenderPlugin::createViewportResources(trc::ViewportContext& ctx)
    -> u_ptr<trc::ViewportResources>
{
    return std::make_unique<ViewportConfig>(*this, ctx);
}



MaterialEditorRenderPlugin::ViewportConfig::ViewportConfig(
    MaterialEditorRenderPlugin& parent,
    trc::ViewportContext&)
    :
    parent(parent)
{
}

void MaterialEditorRenderPlugin::ViewportConfig::registerResources(trc::ResourceStorage& resources)
{
    resources.provideDescriptor(trc::DescriptorName{ kCameraDescriptor },
                                parent.cameraDesc);
    resources.provideDescriptor(trc::DescriptorName{ kTextureDescriptor },
                                parent.renderer.getTextureDescriptor());
}

void MaterialEditorRenderPlugin::ViewportConfig::hostUpdate(trc::ViewportContext& ctx)
{
    parent.cameraDesc->update(ctx.camera());

    auto& scene = ctx.scene().getModule<GraphScene>();
    const auto renderData = buildRenderData(scene);
    parent.renderer.uploadData(renderData);
}

void MaterialEditorRenderPlugin::ViewportConfig::createTasks(
    trc::ViewportDrawTaskQueue& queue,
    trc::ViewportContext&)
{
    queue.spawnTask(
        parent.kMainRenderStage,
        [this](vk::CommandBuffer cmdBuf, trc::ViewportDrawContext& ctx) {
            parent.renderPass.begin(cmdBuf, ctx);
            parent.renderer.draw(cmdBuf, ctx.resources());
            parent.renderPass.end(cmdBuf, ctx);
        }
    );
}
