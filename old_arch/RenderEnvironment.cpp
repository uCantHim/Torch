#include "RenderEnvironment.h"

#include <glm/glm.hpp>



Renderpass::Renderpass(const RenderpassCreateInfo& info)
    :
    renderpass([&]() {
        std::vector<vk::AttachmentDescription> attachments = {
            {
                vk::AttachmentDescriptionFlags(),
                getSwapchain().getImageFormat(),
                vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, // load/store ops
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil ops
                vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
            },
            {
                vk::AttachmentDescriptionFlags(),
                vk::Format::eD32Sfloat,
                vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, // load/store ops
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil ops
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
            },
        };
        return getDevice()->createRenderPassUnique(
            vk::RenderPassCreateInfo(
                info.flags,
                static_cast<uint32_t>(attachments.size()), attachments.data(),
                static_cast<uint32_t>(info.subpasses.size()), info.subpasses.data(),
                static_cast<uint32_t>(info.dependencies.size()), info.dependencies.data()
            )
        );
    }()),
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
    std::vector<vk::ClearValue> clearValues = {
        { std::array<float, 4>{ 0.5f, 0.0f, 0.3f, 1.0f } },
        { vk::ClearDepthStencilValue(1.0f, 0) },
    };
    buf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderpass,
            framebuffer.get(),
            vk::Rect2D({ 0, 0 }, getSwapchain().getImageExtent()),
            static_cast<uint32_t>(clearValues.size()), clearValues.data()
        ),
        vk::SubpassContents::eSecondaryCommandBuffers
    );
}



// ---------------------------- //
//        Pipeline classes        //
// ---------------------------- //

PipelineLayout::PipelineLayout(
    std::vector<vk::DescriptorSetLayout> descriptorSets,
    std::vector<vk::PushConstantRange> pushConstants)
    :
    layout(vkb::VulkanBase::getDevice()->createPipelineLayoutUnique({
        {},
        static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(),
        static_cast<uint32_t>(pushConstants.size()), pushConstants.data()
    })),
    descriptorSetLayouts(descriptorSets),
    pushConstantRanges(pushConstants)
{
}

auto PipelineLayout::get() const noexcept -> vk::PipelineLayout
{
    return *layout;
}

auto PipelineLayout::operator*() const noexcept -> vk::PipelineLayout
{
    return *layout;
}

auto PipelineLayout::getDescriptorSetLayouts() const noexcept
    -> const std::vector<vk::DescriptorSetLayout>&
{
    return descriptorSetLayouts;
}

auto PipelineLayout::getPushConstantRanges() const noexcept
    -> const std::vector<vk::PushConstantRange>&
{
    return pushConstantRanges;
}



GraphicsPipeline::GraphicsPipeline(
    const PipelineLayout& layout,
    vk::GraphicsPipelineCreateInfo createInfo)
    :
    layout(layout),
    pipeline(vkb::VulkanBase::getDevice()->createGraphicsPipelineUnique(
        {}, createInfo
    ))
{
}

auto GraphicsPipeline::operator*() const noexcept -> vk::Pipeline
{
    return *pipeline;
}

auto GraphicsPipeline::get() const noexcept -> vk::Pipeline
{
    return *pipeline;
}

void GraphicsPipeline::bind(vk::CommandBuffer cmdBuf) const noexcept
{
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

    for (const auto& [index, set] : staticDescriptorSets)
    {
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *layout,
            index, set,
            {} // dynamic offsets
        );
    }
}

auto GraphicsPipeline::getLayout() const noexcept -> const PipelineLayout&
{
    return layout;
}

void GraphicsPipeline::setStaticDescriptorSet(uint32_t setIndex, vk::DescriptorSet descriptorSet) noexcept
{
    staticDescriptorSets.emplace_back(setIndex, descriptorSet);
}

