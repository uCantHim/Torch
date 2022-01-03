#include "RenderPassDeferred.h"

#include <glm/detail/type_half.hpp>

#include "trc_util/Padding.h"
#include "PipelineDefinitions.h"



trc::RenderPassDeferred::RenderPassDeferred(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    vkb::FrameSpecific<GBuffer>& gBuffer)
    :
    RenderPass(makeVkRenderPass(device), NUM_SUBPASSES),
    depthPixelReadBuffer(
        device,
        sizeof(float),
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
    ),
    swapchain(swapchain),
    gBuffer(gBuffer),
    framebufferSize(gBuffer->getSize()),
    framebuffers(swapchain, [&](ui32 frameIndex)
    {
        const GBuffer& g = gBuffer.getAt(frameIndex);

        std::vector<vk::ImageView> views{
            g.getImageView(GBuffer::eNormals),
            g.getImageView(GBuffer::eAlbedo),
            g.getImageView(GBuffer::eMaterials),
            g.getImageView(GBuffer::eDepth),
        };

        return Framebuffer(device, *renderPass, g.getSize(), {}, std::move(views));
    }),
    clearValues({
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
        vk::ClearColorValue(std::array<ui32, 4>{ 0, 0, 0, 0 }),
        vk::ClearDepthStencilValue(1.0f, 0.0f),
        vk::ClearColorValue(std::array<float, 4>{ 0.5f, 0.0f, 1.0f, 0.0f }),
    }),
    descriptor(device, swapchain, gBuffer)
{
}

void trc::RenderPassDeferred::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    gBuffer->initFrame(cmdBuf);

    // Bring depth image into depthStencil layout
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllGraphics,
        vk::PipelineStageFlagBits::eAllGraphics,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            {},
            vk::AccessFlagBits::eDepthStencilAttachmentRead
            | vk::AccessFlagBits::eDepthStencilAttachmentRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *gBuffer->getImage(GBuffer::eDepth),
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                0, 1, 0, 1
            )
        )
    );

    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            **framebuffers,
            vk::Rect2D({ 0, 0 }, gBuffer->getExtent()),
            clearValues
        ),
        subpassContents
    );

    // Set viewport and scissor
#ifdef TRC_FLIP_Y_PROJECTION
    cmdBuf.setViewport(0,
        vk::Viewport{
            0.0f, float(framebufferSize.y),
            float(framebufferSize.x), -float(framebufferSize.y),
            0.0f, 1.0f
        }
    );
#else
    cmdBuf.setViewport(0,
        vk::Viewport{
            0.0f, 0.0f,
            float(framebufferSize.x), float(framebufferSize.y),
            0.0f, 1.0f
        }
    );
#endif

    cmdBuf.setScissor(0, vk::Rect2D{ { 0, 0 }, { framebufferSize.x, framebufferSize.y } });
}

void trc::RenderPassDeferred::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRenderPass();
    copyMouseDataToBuffers(cmdBuf);
}

auto trc::RenderPassDeferred::getDescriptor() const noexcept
    -> const DeferredRenderPassDescriptor&
{
    return descriptor;
}

auto trc::RenderPassDeferred::getDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return descriptor.getProvider();
}

auto trc::RenderPassDeferred::makeVkRenderPass(const vkb::Device& device)
    -> vk::UniqueRenderPass
{
    std::vector<vk::AttachmentDescription> attachments = {
        // Normals
        vk::AttachmentDescription(
            {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
        ),
        // Albedo
        vk::AttachmentDescription(
            {}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
        ),
        // Material indices
        vk::AttachmentDescription(
            {}, vk::Format::eR32Uint, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
        ),
        // Depth-/Stencil buffer
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            vk::Format::eD24UnormS8Uint,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, // load/store ops
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, // stencil ops
            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
        ),
    };

    std::vector<vk::AttachmentReference> deferredOutput = {
        { 0, vk::ImageLayout::eColorAttachmentOptimal }, // Normals
        { 1, vk::ImageLayout::eColorAttachmentOptimal }, // UVs
        { 2, vk::ImageLayout::eColorAttachmentOptimal }, // Material indices
    };
    vk::AttachmentReference depthAttachment{ 3, vk::ImageLayout::eDepthStencilAttachmentOptimal };

    std::vector<vk::SubpassDescription> subpasses = {
        // Deferred diffuse subpass
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            {}, // input attachments
            deferredOutput,
            {}, // resolve attachments
            &depthAttachment
        ),
        // Deferred transparency subpass
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            0, nullptr, // input attachments
            0, nullptr, // color attachments
            nullptr, // resolve attachments
            &depthAttachment
        ),
    };

    std::vector<vk::SubpassDependency> dependencies = {
        // The second subpass uses the first's depth buffer for early fragment discard
        vk::SubpassDependency(
            0, 1,
            vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eDepthStencilAttachmentRead,
            vk::DependencyFlagBits::eByRegion
        ),
        // From transparency subpass to final lighting compute pass
        vk::SubpassDependency(
            1, VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::DependencyFlagBits::eByRegion
        )
    };

    return device->createRenderPassUnique(
        vk::RenderPassCreateInfo({}, attachments, subpasses, dependencies)
    );
}

void trc::RenderPassDeferred::copyMouseDataToBuffers(vk::CommandBuffer cmdBuf)
{
    vkb::Image& depthImage = gBuffer.getAt(swapchain.getCurrentFrame()).getImage(GBuffer::eDepth);
    const ivec2 size{ depthImage.getSize().width, depthImage.getSize().height };
    const ivec2 mousePos = glm::clamp(ivec2(swapchain.getMousePosition()), ivec2(0), size - 1);

    depthImage.changeLayout(cmdBuf,
        vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal,
        { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
    );
    cmdBuf.copyImageToBuffer(
        *depthImage, vk::ImageLayout::eTransferSrcOptimal,
        *depthPixelReadBuffer,
        vk::BufferImageCopy(
            0, // buffer offset
            0, 0, // some weird 2D or 3D offsets, idk
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eDepth, 0, 0, 1),
            { mousePos.x, mousePos.y, 0 },
            { 1, 1, 1 }
        )
    );
    depthImage.changeLayout(cmdBuf,
        vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        { vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 }
    );
}

auto trc::RenderPassDeferred::getMouseDepth() const noexcept -> float
{
    const ui32 depthValueD24S8 = depthBufMap[0];

    // Don't ask me why 16 bit here, I think it should be 24. The result is
    // correct when we use 65536 as depth 1.0 (maximum depth) though.
    constexpr float maxFloat16 = 65536.0f;  // 2^16
    return static_cast<float>(depthValueD24S8 >> 8) / maxFloat16;
}

auto trc::RenderPassDeferred::getMousePos(const Camera& camera) const noexcept -> vec3
{
    const vec2 resolution{ gBuffer.getAt(0).getSize() };
#ifdef TRC_FLIP_Y_PROJECTION
    const vec2 mousePos = { swapchain.getMousePosition().x, resolution.y - swapchain.getMousePosition().y, };
#else
    const vec2 mousePos = swapchain.getMousePosition();
#endif
    const float depth = getMouseDepth();

    const vec4 clipSpace = vec4(mousePos / resolution * 2.0f - 1.0f, depth, 1.0);
    const vec4 viewSpace = glm::inverse(camera.getProjectionMatrix()) * clipSpace;
    const vec4 worldSpace = glm::inverse(camera.getViewMatrix()) * (viewSpace / viewSpace.w);

    return worldSpace;
}



// -------------------------------- //
//       Deferred descriptor        //
// -------------------------------- //

trc::DeferredRenderPassDescriptor::DeferredRenderPassDescriptor(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    const vkb::FrameSpecific<GBuffer>& gBuffer)
    :
    descSets(swapchain),
    provider({}, { swapchain }) // Doesn't have a default constructor
{
    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eStorageImage, 5 },
        { vk::DescriptorType::eStorageBuffer, 2 },
        { vk::DescriptorType::eCombinedImageSampler, 1 },
    };
    descPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            swapchain.getFrameCount(), poolSizes)
    );

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        // G-Buffer images
        { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
        { 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
        { 2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
        { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
        // Fragment list head pointer image
        { 4, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eFragment
                                                   | vk::ShaderStageFlagBits::eCompute },
        // Fragment list allocator
        { 5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment
                                                    | vk::ShaderStageFlagBits::eCompute },
        // Fragment list
        { 6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment
                                                    | vk::ShaderStageFlagBits::eCompute },
        // Swapchain image
        { 7, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute },
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Sets
    descSets = { swapchain, [&](ui32 imageIndex)
    {
        auto& g = gBuffer.getAt(imageIndex);
        const auto transparent = g.getTransparencyResources();
        std::vector<vk::ImageView> imageViews{
            g.getImageView(GBuffer::eNormals),
            g.getImageView(GBuffer::eAlbedo),
            g.getImageView(GBuffer::eMaterials),
            g.getImageView(GBuffer::eDepth),
        };
        vk::Sampler depthSampler = g.getImage(GBuffer::eDepth).getDefaultSampler();

        auto set = std::move(device->allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
        )[0]);

        // Write set
        std::vector<vk::DescriptorImageInfo> imageInfos = {
            { {}, imageViews[0], vk::ImageLayout::eGeneral },
            { {}, imageViews[1], vk::ImageLayout::eGeneral },
            { {}, imageViews[2], vk::ImageLayout::eGeneral },
            { depthSampler, imageViews[3], vk::ImageLayout::eShaderReadOnlyOptimal },
            { {}, transparent.headPointerImageView, vk::ImageLayout::eGeneral },
            { {}, swapchain.getImageView(imageIndex), vk::ImageLayout::eGeneral },
        };
        std::vector<vk::DescriptorBufferInfo> bufferInfos{
            { transparent.allocatorAndFragmentListBuf, 0, transparent.FRAGMENT_LIST_OFFSET },
            { transparent.allocatorAndFragmentListBuf, transparent.FRAGMENT_LIST_OFFSET,
                                                       transparent.FRAGMENT_LIST_SIZE },
        };
        std::vector<vk::WriteDescriptorSet> writes = {
            { *set, 0, 0, vk::DescriptorType::eStorageImage, imageInfos[0] },
            { *set, 1, 0, vk::DescriptorType::eStorageImage, imageInfos[1] },
            { *set, 2, 0, vk::DescriptorType::eStorageImage, imageInfos[2] },
            { *set, 3, 0, vk::DescriptorType::eCombinedImageSampler, imageInfos[3] },

            { *set, 4, 0, vk::DescriptorType::eStorageImage, imageInfos[4] },
            { *set, 5, 0, vk::DescriptorType::eStorageBuffer, {}, bufferInfos[0] },
            { *set, 6, 0, vk::DescriptorType::eStorageBuffer, {}, bufferInfos[1] },

            { *set, 7, 0, vk::DescriptorType::eStorageImage, imageInfos[5] },
        };
        device->updateDescriptorSets(writes, {});

        return set;
    }};

    provider = {
        *descLayout,
        { swapchain, [this](ui32 imageIndex) { return *descSets.getAt(imageIndex); } }
    };
}

auto trc::DeferredRenderPassDescriptor::getProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return provider;
}
