#include "RenderStageDeferred.h"

#include "PipelineDefinitions.h"



trc::DeferredStage::DeferredStage()
    :
    RenderStage(2)
{
    RenderPass::create<RenderPassDeferred>(internal::RenderPasses::eDeferredPass);
    addRenderPass(internal::RenderPasses::eDeferredPass);
}



trc::RenderPassDeferred::RenderPassDeferred()
    :
    RenderPass(
        [&]()
        {
            std::vector<vk::AttachmentDescription> attachments = {
                // Vertex positions
                vk::AttachmentDescription(
                    {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal
                ),
                // Normals
                vk::AttachmentDescription(
                    {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal
                ),
                // UVs
                vk::AttachmentDescription(
                    {}, vk::Format::eR16G16Sfloat, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal
                ),
                // Material indices
                vk::AttachmentDescription(
                    {}, vk::Format::eR32Uint, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal
                ),
                // Depth-/Stencil buffer
                trc::makeDefaultDepthStencilAttachment(),
                // Swapchain images
                trc::makeDefaultSwapchainColorAttachment(vkb::VulkanBase::getSwapchain()),
            };

            std::vector<vk::AttachmentReference> deferredAttachments = {
                { 0, vk::ImageLayout::eColorAttachmentOptimal }, // Vertex positions
                { 1, vk::ImageLayout::eColorAttachmentOptimal }, // Normals
                { 2, vk::ImageLayout::eColorAttachmentOptimal }, // UVs
                { 3, vk::ImageLayout::eColorAttachmentOptimal }, // Material indices
                { 4, vk::ImageLayout::eDepthStencilAttachmentOptimal }, // Depth buffer
            };

            std::vector<vk::AttachmentReference> lightingAttachments = {
                { 0, vk::ImageLayout::eShaderReadOnlyOptimal }, // Vertex positions
                { 1, vk::ImageLayout::eShaderReadOnlyOptimal }, // Normals
                { 2, vk::ImageLayout::eShaderReadOnlyOptimal }, // UVs
                { 3, vk::ImageLayout::eShaderReadOnlyOptimal }, // Material indices
                { 5, vk::ImageLayout::eColorAttachmentOptimal }, // Swapchain images
            };

            std::vector<vk::SubpassDescription> subpasses = {
                vk::SubpassDescription(
                    vk::SubpassDescriptionFlags(),
                    vk::PipelineBindPoint::eGraphics,
                    0, nullptr,
                    4, &deferredAttachments[0],
                    nullptr, // resolve attachments
                    &deferredAttachments[4]
                ),
                vk::SubpassDescription(
                    vk::SubpassDescriptionFlags(),
                    vk::PipelineBindPoint::eGraphics,
                    4, &lightingAttachments[0],
                    1, &lightingAttachments[4],
                    nullptr, // resolve attachments
                    nullptr
                ),
            };

            std::vector<vk::SubpassDependency> dependencies = {
                vk::SubpassDependency(
                    VK_SUBPASS_EXTERNAL, 0,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::AccessFlags(),
                    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                    vk::DependencyFlags()
                ),
                vk::SubpassDependency(
                    0, 1,
                    vk::PipelineStageFlagBits::eAllGraphics,
                    vk::PipelineStageFlagBits::eAllGraphics,
                    vk::AccessFlagBits::eColorAttachmentWrite,
                    vk::AccessFlagBits::eInputAttachmentRead,
                    vk::DependencyFlagBits::eByRegion
                )
            };

            return vkb::getDevice()->createRenderPassUnique(
                vk::RenderPassCreateInfo(
                    vk::RenderPassCreateFlags(),
                    static_cast<ui32>(attachments.size()), attachments.data(),
                    static_cast<ui32>(subpasses.size()), subpasses.data(),
                    static_cast<ui32>(dependencies.size()), dependencies.data()
                )
            );
        }(),
        2 // Number of subpasses
    ), // Base class RenderPass constructor
    framebuffers([&](ui32 frameIndex)
    {
        constexpr size_t numAttachments = 5;

        const auto swapchainExtent = vkb::getSwapchain().getImageExtent();
        auto& images = attachmentImages.emplace_back();
        images.reserve(numAttachments);
        auto& imageViews = attachmentImageViews.emplace_back();

        auto& positionImage = images.emplace_back(vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR16G16B16A16Sfloat,
            vk::Extent3D{ swapchainExtent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment
        ));
        auto& normalImage = images.emplace_back(vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR16G16B16A16Sfloat,
            vk::Extent3D{ swapchainExtent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment
        ));
        auto& uvImage = images.emplace_back(vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR16G16Sfloat,
            vk::Extent3D{ swapchainExtent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment
        ));
        auto& materialImage = images.emplace_back(vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR32Uint,
            vk::Extent3D{ swapchainExtent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment
        ));
        auto& depthImage = images.emplace_back(vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eD24UnormS8Uint,
            vk::Extent3D{ swapchainExtent, 1 },
            1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment
        ));

        auto positionView = *imageViews.emplace_back(
            positionImage.createView(vk::ImageViewType::e2D, vk::Format::eR16G16B16A16Sfloat, {})
        );
        auto normalView = *imageViews.emplace_back(
            normalImage.createView(vk::ImageViewType::e2D, vk::Format::eR16G16B16A16Sfloat, {})
        );
        auto uvView = *imageViews.emplace_back(
            uvImage.createView(vk::ImageViewType::e2D, vk::Format::eR16G16Sfloat, {})
        );
        auto materialView = *imageViews.emplace_back(
            materialImage.createView(vk::ImageViewType::e2D, vk::Format::eR32Uint, {})
        );
        auto depthView = *imageViews.emplace_back(
            depthImage.createView(
                vk::ImageViewType::e2D, vk::Format::eD24UnormS8Uint, vk::ComponentMapping(),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
            )
        );
        auto colorOutputView = vkb::getSwapchain().getImageView(frameIndex);

        std::vector<vk::ImageView> attachments = {
            positionView, normalView, uvView, materialView, depthView, colorOutputView
        };

        framebufferSize = vk::Extent2D{ swapchainExtent.width, swapchainExtent.height };
        return vkb::getDevice()->createFramebufferUnique(
            vk::FramebufferCreateInfo(
                {},
                *renderPass,
                attachments,
                framebufferSize.width, framebufferSize.height, 1
            )
        );
    }),
    clearValues({}),
    descriptorProvider(*this)
{
    clearValues = {
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<ui32, 4>{ UINT32_MAX, 0, 0, 0 }),
        vk::ClearDepthStencilValue(1.0f, 0.0f),
        vk::ClearColorValue(std::array<float, 4>{ 0.5f, 0.0f, 1.0f, 0.0f }),
    };

    createInputAttachmentDescriptors();
}

void trc::RenderPassDeferred::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    // Bring attachment images in color attachment layout
    ui32 imageIndex = vkb::getSwapchain().getCurrentFrame();
    auto& images = attachmentImages[imageIndex];
    images[0].changeLayout(cmdBuf, vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal);
    images[1].changeLayout(cmdBuf, vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal);
    images[2].changeLayout(cmdBuf, vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal);
    images[3].changeLayout(cmdBuf, vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal);

    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            **framebuffers,
            vk::Rect2D(
                { 0, 0 },
                framebufferSize
            ),
            static_cast<ui32>(clearValues.size()), clearValues.data()
        ),
        subpassContents
    );
}

void trc::RenderPassDeferred::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRenderPass();
}

auto trc::RenderPassDeferred::getInputAttachmentDescriptor() const noexcept
    -> const DescriptorProviderInterface&
{
    return descriptorProvider;
}

void trc::RenderPassDeferred::createInputAttachmentDescriptors()
{
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eInputAttachment, 3 },
    };
    descPool = vkb::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            vkb::getSwapchain().getFrameCount(), poolSizes)
    );

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        { 0, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
        { 1, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
        { 2, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
        { 3, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
    };
    inputAttachmentLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Sets
    inputAttachmentSets = { [&](ui32 imageIndex)
    {
        const auto& imageViews = attachmentImageViews[imageIndex];

        auto set = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo(*descPool, *inputAttachmentLayout)
        )[0]);

        // Write set
        std::vector<vk::DescriptorImageInfo> imageInfos = {
            vk::DescriptorImageInfo({}, *imageViews[0], vk::ImageLayout::eShaderReadOnlyOptimal),
            vk::DescriptorImageInfo({}, *imageViews[1], vk::ImageLayout::eShaderReadOnlyOptimal),
            vk::DescriptorImageInfo({}, *imageViews[2], vk::ImageLayout::eShaderReadOnlyOptimal),
            vk::DescriptorImageInfo({}, *imageViews[3], vk::ImageLayout::eShaderReadOnlyOptimal),
        };
        std::vector<vk::WriteDescriptorSet> writes = {
            { *set, 0, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[0] },
            { *set, 1, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[1] },
            { *set, 2, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[2] },
            { *set, 3, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[3] },
        };

        vkb::getDevice()->updateDescriptorSets(writes, {});

        return set;
    }};
}

auto trc::RenderPassDeferred::InputAttachmentDescriptorProvider::getDescriptorSet()
    const noexcept -> vk::DescriptorSet
{
    return **renderPass->inputAttachmentSets;
}

auto trc::RenderPassDeferred::InputAttachmentDescriptorProvider::getDescriptorSetLayout()
    const noexcept -> vk::DescriptorSetLayout
{
    return *renderPass->inputAttachmentLayout;
}

void trc::RenderPassDeferred::InputAttachmentDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    cmdBuf.bindDescriptorSets(bindPoint, pipelineLayout, setIndex, getDescriptorSet(), {});
}
