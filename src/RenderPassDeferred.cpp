#include "RenderPassDeferred.h"

#include <glm/detail/type_half.hpp>

#include "util/Padding.h"
#include "PipelineDefinitions.h"



trc::RenderPassDeferred::RenderPassDeferred(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    const RenderPassDeferredCreateInfo& info)
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
    framebufferSize(info.gBufferSize.x, info.gBufferSize.y),
    gBuffers(swapchain, [&](ui32) {
        return GBuffer(device, { info.gBufferSize });
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
    descriptor(device, swapchain, gBuffers)
{
}

void trc::RenderPassDeferred::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    gBuffers->clearTransparencyBufferData(cmdBuf);

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
            *gBuffers.get().getImage(GBuffer::eDepth),
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

auto trc::RenderPassDeferred::getGBuffer() -> vkb::FrameSpecific<GBuffer>&
{
    return gBuffers;
}

auto trc::RenderPassDeferred::getGBuffer() const -> const vkb::FrameSpecific<GBuffer>&
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
    const vec2 resolution{ framebufferSize.width, framebufferSize.height };
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
    const vkb::FrameSpecific<GBuffer>& gBuffer)
    :
    descSets(swapchain),
    provider({}, { swapchain }) // Doesn't have a default constructor
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
        auto& g = gBuffer.getAt(imageIndex);
        const auto transparent = g.getTransparencyResources();
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
            { {}, transparent.headPointerImageView, vk::ImageLayout::eGeneral },
        };
        std::vector<vk::DescriptorBufferInfo> bufferInfos{
            { transparent.allocatorAndFragmentListBuf, 0, transparent.FRAGMENT_LIST_OFFSET },
            { transparent.allocatorAndFragmentListBuf,
              transparent.FRAGMENT_LIST_OFFSET, transparent.FRAGMENT_LIST_SIZE },
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

auto trc::DeferredRenderPassDescriptor::getProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return provider;
}
