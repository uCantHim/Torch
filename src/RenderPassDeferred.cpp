#include "RenderPassDeferred.h"

#include <glm/detail/type_half.hpp>

#include "utils/Util.h"
#include "PipelineDefinitions.h"



trc::RenderPassDeferred::RenderPassDeferred(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    const ui32 maxTransparentFragsPerPixel)
    :
    RenderPass(
        makeVkRenderPass(device, swapchain),
        NUM_SUBPASSES
    ), // Base class RenderPass constructor
    depthPixelReadBuffer(
        device,
        sizeof(float),
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
    ),
    swapchain(swapchain),
    framebufferSize(swapchain.getImageExtent()),
    gBuffers(swapchain, [&](ui32) {
        const auto extent = swapchain.getImageExtent();
        return GBuffer(device, uvec2(extent.width, extent.height));
    }),
    framebuffers(swapchain, [&](ui32 frameIndex)
    {
        GBuffer& g = gBuffers.getAt(frameIndex);

        std::vector<vk::ImageView> views{
            g.getImageView(GBuffer::eNormals),
            g.getImageView(GBuffer::eAlbedo),
            g.getImageView(GBuffer::eMaterials),
            g.getImageView(GBuffer::eDepth),
            swapchain.getImageView(frameIndex),
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
    descriptor(device, swapchain, *this, maxTransparentFragsPerPixel)
{
}

void trc::RenderPassDeferred::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    descriptor.resetValues(cmdBuf);

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
    copyMouseDataToBuffers(cmdBuf);
}

auto trc::RenderPassDeferred::getGBuffer() -> vkb::FrameSpecificObject<GBuffer>&
{
    return gBuffers;
}

auto trc::RenderPassDeferred::getGBuffer() const -> const vkb::FrameSpecificObject<GBuffer>&
{
    return gBuffers;
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

auto trc::RenderPassDeferred::makeVkRenderPass(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain)
    -> vk::UniqueRenderPass
{
    std::vector<vk::AttachmentDescription> attachments = {
        // Normals
        vk::AttachmentDescription(
            {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
        ),
        // Albedo
        vk::AttachmentDescription(
            {}, vk::Format::eR32Uint, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
        ),
        // Material indices
        vk::AttachmentDescription(
            {}, vk::Format::eR32Uint, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
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
        // Swapchain images
        trc::makeDefaultSwapchainColorAttachment(swapchain),
    };

    std::vector<vk::AttachmentReference> deferredOutput = {
        { 0, vk::ImageLayout::eColorAttachmentOptimal }, // Normals
        { 1, vk::ImageLayout::eColorAttachmentOptimal }, // UVs
        { 2, vk::ImageLayout::eColorAttachmentOptimal }, // Material indices
    };
    vk::AttachmentReference deferredDepth{ 3, vk::ImageLayout::eDepthStencilAttachmentOptimal };

    std::vector<vk::AttachmentReference> transparencyAttachments{
        { 3, vk::ImageLayout::eDepthStencilAttachmentOptimal }, // Depth buffer
    };
    std::vector<ui32> transparencyPreservedAttachments{ 0, 1, 2 };

    std::vector<vk::AttachmentReference> lightingInput = {
        { 0, vk::ImageLayout::eShaderReadOnlyOptimal }, // Normals
        { 1, vk::ImageLayout::eShaderReadOnlyOptimal }, // UVs
        { 2, vk::ImageLayout::eShaderReadOnlyOptimal }, // Material indices
        { 3, vk::ImageLayout::eShaderReadOnlyOptimal }, // Depth
    };
    vk::AttachmentReference lightingColor{ 4, vk::ImageLayout::eColorAttachmentOptimal };

    std::vector<vk::SubpassDescription> subpasses = {
        // Deferred diffuse subpass
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            0, nullptr,
            deferredOutput.size(), deferredOutput.data(),
            nullptr, // resolve attachments
            &deferredDepth
        ),
        // Deferred transparency subpass
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            0, nullptr, // input attachments
            0, nullptr, // color attachments
            nullptr,    // resolve attachments
            &transparencyAttachments[0], // depth attachment (read-only)
            transparencyPreservedAttachments.size(), transparencyPreservedAttachments.data()
        ),
        // Final lighting subpass
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            lightingInput.size(), lightingInput.data(),  // input attachments
            1, &lightingColor,  // color attachments
            nullptr, // resolve attachments
            nullptr  // depth attachment
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
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eDepthStencilAttachmentRead,
            vk::DependencyFlagBits::eByRegion
        ),
        vk::SubpassDependency(
            1, 2,
            vk::PipelineStageFlagBits::eAllGraphics,
            vk::PipelineStageFlagBits::eAllGraphics,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eInputAttachmentRead,
            vk::DependencyFlagBits::eByRegion
        )
    };

    return device->createRenderPassUnique(
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            static_cast<ui32>(attachments.size()), attachments.data(),
            static_cast<ui32>(subpasses.size()), subpasses.data(),
            static_cast<ui32>(dependencies.size()), dependencies.data()
        )
    );
}

void trc::RenderPassDeferred::copyMouseDataToBuffers(vk::CommandBuffer cmdBuf)
{
    vkb::Image& depthImage = gBuffers.getAt(swapchain.getCurrentFrame()).getImage(GBuffer::eDepth);
    const ivec2 size{ depthImage.getSize().width, depthImage.getSize().height };
    const ivec2 mousePos = glm::clamp(ivec2(swapchain.getMousePosition()), ivec2(0), size - 1);

    depthImage.changeLayout(
        cmdBuf,
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
            { static_cast<i32>(mousePos.x), static_cast<i32>(mousePos.y), 0 },
            { 1, 1, 1 }
        )
    );
}

auto trc::RenderPassDeferred::getMouseDepth() const noexcept -> float
{
    return depthBufMap[0];

    const ui32 depthValueD24S8 = depthBufMap[0];

    // Don't ask me why 16 bit here, I think it should be 24. The result
    // seems to be correct with 16 though.
    constexpr float maxFloat16 = 65536.0f; // 2**16 -- std::pow is not constexpr
    return static_cast<float>(depthValueD24S8 >> 8) / maxFloat16;
}

auto trc::RenderPassDeferred::getMousePos(const Camera& camera) const noexcept -> vec3
{
    const vec2 resolution{ swapchain.getImageExtent().width, swapchain.getImageExtent().height };
    const vec2 mousePos = swapchain.getMousePosition();
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
    const RenderPassDeferred& renderPass,
    const ui32 maxTransparentFragsPerPixel)
    :
    ATOMIC_BUFFER_SECTION_SIZE(util::pad(
        sizeof(ui32),
        device.getPhysicalDevice().properties.limits.minStorageBufferOffsetAlignment
    )),
    FRAG_LIST_BUFFER_SIZE(
        sizeof(uvec4) * maxTransparentFragsPerPixel
        * swapchain.getImageExtent().width * swapchain.getImageExtent().height
    ),
    fragmentListHeadPointerImage(swapchain),
    fragmentListHeadPointerImageView(swapchain),
    fragmentListBuffer(swapchain),
    descSets(swapchain),
    provider({}, { swapchain }) // Doesn't have a default constructor
{
    createFragmentList(device, swapchain, maxTransparentFragsPerPixel);
    createDescriptors(device, swapchain, renderPass);
}

void trc::DeferredRenderPassDescriptor::resetValues(vk::CommandBuffer cmdBuf) const
{
    cmdBuf.copyBuffer(**fragmentListBuffer, **fragmentListBuffer,
                      vk::BufferCopy(sizeof(ui32) * 3, 0, sizeof(ui32)));
}

auto trc::DeferredRenderPassDescriptor::getProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return provider;
}

void trc::DeferredRenderPassDescriptor::createFragmentList(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    const ui32 maxFragsPerPixel)
{
    const vk::Extent2D swapchainSize = swapchain.getImageExtent();

    // Fragment list
    std::vector<vk::UniqueImageView> imageViews;
    fragmentListHeadPointerImage = { swapchain, [&](ui32)
    {
        vkb::Image result(
            device,
            vk::ImageCreateInfo(
                {},
                vk::ImageType::e2D, vk::Format::eR32Uint,
                vk::Extent3D(swapchainSize.width, swapchainSize.height, 1),
                1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst
            ),
            vkb::DefaultDeviceMemoryAllocator()
        );
        result.changeLayout(device, vk::ImageLayout::eGeneral);
        imageViews.push_back(result.createView(vk::ImageViewType::e2D, vk::Format::eR32Uint));

        // Clear image
        auto cmdBuf = device.createGraphicsCommandBuffer();
        cmdBuf->begin(vk::CommandBufferBeginInfo());
        cmdBuf->clearColorImage(
            *result, vk::ImageLayout::eGeneral,
            vk::ClearColorValue(std::array<ui32, 4>{ ~0u }),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );
        cmdBuf->end();
        device.executeGraphicsCommandBufferSynchronously(*cmdBuf);

        return result;
    }};

    fragmentListHeadPointerImageView = { swapchain, std::move(imageViews) };

    fragmentListBuffer = { swapchain, [&](ui32)
    {
        vkb::Buffer result(
            device,
            ATOMIC_BUFFER_SECTION_SIZE + FRAG_LIST_BUFFER_SIZE,
            vk::BufferUsageFlagBits::eStorageBuffer
                | vk::BufferUsageFlagBits::eTransferDst
                | vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        const ui32 MAX_FRAGS = maxFragsPerPixel * swapchainSize.width * swapchainSize.height;
        auto cmdBuf = device.createTransferCommandBuffer();
        cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferBeginInfo()));
        cmdBuf->updateBuffer<ui32>(*result, 0, { 0, MAX_FRAGS, 0, });
        cmdBuf->end();
        device.executeTransferCommandBufferSyncronously(*cmdBuf);

        return result;
    }};
}

void trc::DeferredRenderPassDescriptor::createDescriptors(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    const RenderPassDeferred& renderPass)
{
    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eInputAttachment, 4 },
        { vk::DescriptorType::eStorageImage, 1 },
        { vk::DescriptorType::eStorageBuffer, 2 },
    };
    descPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            swapchain.getFrameCount(), poolSizes)
    );

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        // Input attachments
        { 0, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
        { 1, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
        { 2, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
        { 3, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment },
        // Fragment list head pointer image
        { 4, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eFragment },
        // Fragment list allocator
        { 5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
        // Fragment list
        { 6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Sets
    descSets = { swapchain, [&](ui32 imageIndex)
    {
        auto& g = renderPass.getGBuffer().getAt(imageIndex);
        std::vector<vk::ImageView> imageViews{
            g.getImageView(GBuffer::eNormals),
            g.getImageView(GBuffer::eAlbedo),
            g.getImageView(GBuffer::eMaterials),
            g.getImageView(GBuffer::eDepth),
        };

        auto set = std::move(device->allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
        )[0]);

        // Write set
        std::vector<vk::DescriptorImageInfo> imageInfos = {
            { {}, imageViews[0], vk::ImageLayout::eShaderReadOnlyOptimal },
            { {}, imageViews[1], vk::ImageLayout::eShaderReadOnlyOptimal },
            { {}, imageViews[2], vk::ImageLayout::eShaderReadOnlyOptimal },
            { {}, imageViews[3], vk::ImageLayout::eShaderReadOnlyOptimal },
            { {}, *fragmentListHeadPointerImageView.getAt(imageIndex), vk::ImageLayout::eGeneral },
        };
        std::vector<vk::DescriptorBufferInfo> bufferInfos{
            { *fragmentListBuffer.getAt(imageIndex),
              0, ATOMIC_BUFFER_SECTION_SIZE },
            { *fragmentListBuffer.getAt(imageIndex),
              ATOMIC_BUFFER_SECTION_SIZE, FRAG_LIST_BUFFER_SIZE },
        };
        std::vector<vk::WriteDescriptorSet> writes = {
            { *set, 0, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[0] },
            { *set, 1, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[1] },
            { *set, 2, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[2] },
            { *set, 3, 0, 1, vk::DescriptorType::eInputAttachment, &imageInfos[3] },
            { *set, 4, 0, 1, vk::DescriptorType::eStorageImage, &imageInfos[4] },
            { *set, 5, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &bufferInfos[0] },
            { *set, 6, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &bufferInfos[1] },
        };
        device->updateDescriptorSets(writes, {});

        return set;
    }};

    provider = {
        *descLayout,
        { swapchain, [this](ui32 imageIndex) { return *descSets.getAt(imageIndex); } }
    };
}
