#include "GBuffer.h"

#include "trc_util/Padding.h"
#include "DescriptorSetUtils.h"
#include "Camera.h"



trc::GBuffer::GBuffer(const vkb::Device& device, const GBufferCreateInfo& info)
    :
    size(info.size),
    extent(size.x, size.y),
    ATOMIC_BUFFER_SECTION_SIZE(util::pad(
        sizeof(ui32) * 3,  // current index, max index, clear value
        device.getPhysicalDevice().properties.limits.minStorageBufferOffsetAlignment
    )),
    FRAG_LIST_BUFFER_SIZE(
        sizeof(uvec4) * info.maxTransparentFragsPerPixel * info.size.x * info.size.y
    ),
    fragmentListHeadPointerImage(
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D, vk::Format::eR32Uint,
            vk::Extent3D(size.x, size.y, 1),
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst
        ),
        vkb::DefaultDeviceMemoryAllocator()
    ),
    fragmentListHeadPointerImageView(
        fragmentListHeadPointerImage.createView(vk::ImageViewType::e2D, vk::Format::eR32Uint)
    ),
    fragmentListBuffer(
        device,
        ATOMIC_BUFFER_SECTION_SIZE + FRAG_LIST_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eTransferDst
            | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    )
{
    // Clear fragment head pointer image
    device.executeCommandsSynchronously(vkb::QueueType::graphics, [this](auto cmdBuf)
    {
        fragmentListHeadPointerImage.changeLayout(cmdBuf,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        );
        cmdBuf.clearColorImage(
            *fragmentListHeadPointerImage, vk::ImageLayout::eGeneral,
            vk::ClearColorValue(std::array<ui32, 4>{ ~0u }),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );
    });

    // Clear atomic counter buffer
    device.executeCommandsSynchronously(vkb::QueueType::transfer, [&, this](vk::CommandBuffer cmdBuf)
    {
        const ui32 MAX_FRAGS = info.maxTransparentFragsPerPixel * info.size.x * info.size.y;
        cmdBuf.updateBuffer<ui32>(*fragmentListBuffer, 0, { 0, MAX_FRAGS, 0, });
    });


    ///////////////////////////
    // Create attachment images

    images.reserve(Image::NUM_IMAGES);
    const auto colorUsage = vk::ImageUsageFlagBits::eColorAttachment
                            | vk::ImageUsageFlagBits::eInputAttachment
                            | vk::ImageUsageFlagBits::eStorage;
    const auto depthUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment
                            | vk::ImageUsageFlagBits::eTransferSrc
                            | vk::ImageUsageFlagBits::eInputAttachment
                            | vk::ImageUsageFlagBits::eSampled;

    // Normals
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR16G16B16A16Sfloat,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            colorUsage
        ),
        vkb::DefaultDeviceMemoryAllocator()
    );

    // Albedo
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            colorUsage
        ),
        vkb::DefaultDeviceMemoryAllocator()
    );

    // Material
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR32Uint,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            colorUsage
        ),
        vkb::DefaultDeviceMemoryAllocator()
    );

    // Depth
    images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eD24UnormS8Uint,
            vk::Extent3D{ extent, 1 },
            1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            depthUsage
        ),
        vkb::DefaultDeviceMemoryAllocator()
    );

    imageViews.push_back(getImage(Image::eNormals).createView());
    imageViews.push_back(getImage(Image::eAlbedo).createView());
    imageViews.push_back(getImage(Image::eMaterials).createView());
    imageViews.push_back(getImage(Image::eDepth).createView(vk::ImageAspectFlagBits::eDepth));
}

auto trc::GBuffer::getSize() const -> uvec2
{
    return size;
}

auto trc::GBuffer::getExtent() const -> vk::Extent2D
{
    return extent;
}

auto trc::GBuffer::getImage(Image imageType) -> vkb::Image&
{
    return images[imageType];
}

auto trc::GBuffer::getImage(Image imageType) const -> const vkb::Image&
{
    return images[imageType];
}

auto trc::GBuffer::getImageView(Image imageType) const -> vk::ImageView
{
    return *imageViews[imageType];
}

auto trc::GBuffer::getTransparencyResources() const -> TransparencyResources
{
    return {
        *fragmentListHeadPointerImageView,
        *fragmentListBuffer,
        ATOMIC_BUFFER_SECTION_SIZE,
        FRAG_LIST_BUFFER_SIZE
    };
}

void trc::GBuffer::initFrame(vk::CommandBuffer cmdBuf) const
{
    // Cheat: Copy a constantly zero-valued byte to the first byte in the buffer
    cmdBuf.copyBuffer(*fragmentListBuffer, *fragmentListBuffer,
                      vk::BufferCopy(sizeof(ui32) * 2, 0, sizeof(ui32)));
    fragmentListBuffer.barrier(
        cmdBuf,
        0, sizeof(ui32),
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eShaderRead
    );
}



// -------------------------------- //
//       G-Buffer descriptor        //
// -------------------------------- //

trc::GBufferDescriptor::GBufferDescriptor(
    const vkb::Device& device,
    const vkb::FrameClock& frameClock)
    :
    descSets(frameClock),
    provider({}, { frameClock }) // Doesn't have a default constructor
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
            frameClock.getFrameCount(), poolSizes)
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
    std::vector<vk::DescriptorSetLayout> layouts(frameClock.getFrameCount(), *descLayout);
    descSets = { frameClock, device->allocateDescriptorSetsUnique({ *descPool, layouts }) };

    provider = {
        *descLayout,
        { frameClock, [this](ui32 imageIndex) { return *descSets.getAt(imageIndex); } }
    };
}

trc::GBufferDescriptor::GBufferDescriptor(
    const vkb::Device& device,
    const vkb::FrameSpecific<GBuffer>& gBuffer)
    :
    GBufferDescriptor(device, gBuffer.getFrameClock())
{
    update(device, gBuffer);
}

auto trc::GBufferDescriptor::getProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return provider;
}

void trc::GBufferDescriptor::update(
    const vkb::Device& device,
    const vkb::FrameSpecific<GBuffer>& gBuffer)
{
    const ui32 frameCount = gBuffer.getFrameClock().getFrameCount();
    for (size_t frame = 0; frame < frameCount; frame++)
    {
        const auto& g = gBuffer.getAt(frame);

        vk::ImageView imageViews[]{
            g.getImageView(GBuffer::eNormals),
            g.getImageView(GBuffer::eAlbedo),
            g.getImageView(GBuffer::eMaterials),
            g.getImageView(GBuffer::eDepth),
        };
        vk::Sampler depthSampler = g.getImage(GBuffer::eDepth).getDefaultSampler();
        const auto transparent = g.getTransparencyResources();

        // Write set
        vk::DescriptorImageInfo imageInfos[]{
            { {},           imageViews[0],                    vk::ImageLayout::eGeneral },
            { {},           imageViews[1],                    vk::ImageLayout::eGeneral },
            { {},           imageViews[2],                    vk::ImageLayout::eGeneral },
            { depthSampler, imageViews[3],                    vk::ImageLayout::eShaderReadOnlyOptimal },
            { {},           transparent.headPointerImageView, vk::ImageLayout::eGeneral },
        };
        vk::DescriptorBufferInfo bufferInfos[]{
            { transparent.allocatorAndFragmentListBuf, 0, transparent.FRAGMENT_LIST_OFFSET },
            { transparent.allocatorAndFragmentListBuf, transparent.FRAGMENT_LIST_OFFSET,
                                                       transparent.FRAGMENT_LIST_SIZE },
        };

        const auto set = *descSets.getAt(frame);
        std::vector<vk::WriteDescriptorSet> writes{
            { set, 0, 0, vk::DescriptorType::eStorageImage,         imageInfos[0] },
            { set, 1, 0, vk::DescriptorType::eStorageImage,         imageInfos[1] },
            { set, 2, 0, vk::DescriptorType::eStorageImage,         imageInfos[2] },
            { set, 3, 0, vk::DescriptorType::eCombinedImageSampler, imageInfos[3] },

            { set, 4, 0, vk::DescriptorType::eStorageImage,         imageInfos[4] },
            { set, 5, 0, vk::DescriptorType::eStorageBuffer,        {}, bufferInfos[0] },
            { set, 6, 0, vk::DescriptorType::eStorageBuffer,        {}, bufferInfos[1] },
        };

        device->updateDescriptorSets(writes, {});
    }
}
