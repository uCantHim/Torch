#include "RenderEnvironment.h"

#include <glm/glm.hpp>



Renderpass::Renderpass(const RenderpassCreateInfo& info)
    :
    renderpass(getDevice()->createRenderPassUnique(
        vk::RenderPassCreateInfo(
            info.flags,
            static_cast<uint32_t>(info.attachments.size()), info.attachments.data(),
            static_cast<uint32_t>(info.subpasses.size()), info.subpasses.data(),
            static_cast<uint32_t>(info.dependencies.size()), info.dependencies.data()
        )
    )),
    framebuffer(*renderpass),
    subpassCount(info.subpasses.size()),
    // Creates synchronization objects
    imageReadySemaphore([](uint32_t) {
        return getDevice()->createSemaphoreUnique({});
    }),
    renderFinishedSemaphore([](uint32_t) {
        return getDevice()->createSemaphoreUnique({});
    }),
    duringRenderFence([](uint32_t) {
        return getDevice()->createFenceUnique(
            vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)
        );
    }),
    staticDescriptorSets(std::move(info.renderpassLocalDescriptorSets)),
    engine(*this)
{
}


Renderpass::~Renderpass()
{
    std::vector<vk::Fence> fences;
    duringRenderFence.foreach([&](vk::UniqueFence& fence) {
        fences.push_back(*fence);
    });
    getDevice()->waitForFences(fences, VK_TRUE, UINT64_MAX);
}


auto Renderpass::getFramebuffer() const noexcept -> const Framebuffer&
{
    return framebuffer;
}


auto Renderpass::getSubpassCount() const noexcept -> size_t
{
    return subpassCount;
}


auto Renderpass::getRenderpassDescriptorSets() const noexcept -> const std::vector<vk::DescriptorSet>&
{
    return staticDescriptorSets;
}


void Renderpass::addRenderpassDescriptorSet(vk::DescriptorSet descriptorSet) noexcept
{
    staticDescriptorSets.push_back(descriptorSet);
}


void Renderpass::drawFrame(Scene& scene)
{
    uint32_t image = getSwapchain().acquireImage(*imageReadySemaphore.get());

    // Wait for the fence of the current frame (i.e. wait for the current image
    // to finish being rendered to)
    getDevice()->waitForFences(*duringRenderFence.get(), true, UINT64_MAX);

    // The engine generates primary command buffers
    auto commandBuffers = engine.recordScene(scene);

    // Submit commandbuffer
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    getDevice()->resetFences(*duringRenderFence.get());
    getQueueProvider().getQueue(vkb::queue_type::graphics).submit(
        vk::SubmitInfo(
            1, &*imageReadySemaphore.get(),
            waitStages, // Stages that will be executed AFTER the waitSemaphores have been reached.
            commandBuffers.size(), commandBuffers.data(),
            1, &*renderFinishedSemaphore.get()
        ),
        *duringRenderFence.get()
    );

    // Present the image
    getSwapchain().presentImage(
        image,
        getQueueProvider().getQueue(vkb::queue_type::presentation),
        { *renderFinishedSemaphore.get() }
    );
}


void Renderpass::beginCommandBuffer(const vk::CommandBuffer& buf) const noexcept
{
    vk::ClearValue clearColor(std::array<float, 4>{0.5f, 0.0f, 0.3f, 1.0f});
    buf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderpass,
            framebuffer.get(),
            vk::Rect2D({ 0, 0 }, getSwapchain().getImageExtent()),
            1u, &clearColor
        ),
        vk::SubpassContents::eSecondaryCommandBuffers
    );
}



// ---------------------------- //
//        Pipeline classes        //
// ---------------------------- //

Pipeline::Pipeline(
    vk::Pipeline pipeline,
    vk::PipelineLayout layout,
    vk::PipelineBindPoint bindPoint)
    :
    bindPoint(bindPoint),
    layout(layout),
    pipeline(pipeline)
{
}

Pipeline::~Pipeline()
{
    vkb::VulkanBase::getDevice()->destroyPipeline(pipeline);
}


auto Pipeline::get() noexcept -> vk::Pipeline&
{
    return pipeline;
}

auto Pipeline::get() const noexcept -> const vk::Pipeline&
{
    return pipeline;
}

auto Pipeline::operator*() const noexcept -> const vk::Pipeline&
{
    return pipeline;
}

auto Pipeline::operator*() noexcept -> vk::Pipeline&
{
    return pipeline;
}

void Pipeline::bind(vk::CommandBuffer cmdBuf) const noexcept
{
    cmdBuf.bindPipeline(bindPoint, pipeline);
    cmdBuf.bindDescriptorSets(
        bindPoint,
        layout,
        0,
        staticDescriptorSets,
        {} // dynamic offsets
    );
}

auto Pipeline::getPipelineLayout() const noexcept -> vk::PipelineLayout
{
    return layout;
}

auto Pipeline::getPipelineDescriptorSets() const noexcept -> const std::vector<vk::DescriptorSet>&
{
    return staticDescriptorSets;
}

void Pipeline::addPipelineDescriptorSet(vk::DescriptorSet descriptorSet) noexcept
{
    staticDescriptorSets.push_back(descriptorSet);
}



GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineCreateInfo& info)
    :
    Pipeline(
        getDevice()->createGraphicsPipeline(
            vk::PipelineCache(),
            vk::GraphicsPipelineCreateInfo(
                info.flags,
                static_cast<uint32_t>(info.shaderStages.size()), info.shaderStages.data(),
                &info.vertexInput,
                &info.inputAssembly,
                &info.tessellation,
                &info.viewport,
                &info.rasterization,
                &info.multisample,
                &info.depthStencil,
                &info.colorBlend,
                &info.dynamicState,
                info.layout,
                *info.renderpass,
                info.subpassIndex,
                vk::Pipeline(), 0
            )
        ).value,
        info.layout,
        vk::PipelineBindPoint::eGraphics
    )
{
}
