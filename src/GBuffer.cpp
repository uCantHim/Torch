#include "GBuffer.h"

#include "util/Padding.h"
#include "DescriptorSetUtils.h"



trc::GBuffer::GBuffer(const vkb::Device& device, const GBufferCreateInfo& info)
    :
    size(info.size),
    extent(size.x, size.y),
    ATOMIC_BUFFER_SECTION_SIZE(util::pad(
        sizeof(ui32),
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
    auto cmdBuf = device.createGraphicsCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo());

    fragmentListHeadPointerImage.changeLayout(*cmdBuf, vk::ImageLayout::eGeneral);
    cmdBuf->clearColorImage(
        *fragmentListHeadPointerImage, vk::ImageLayout::eGeneral,
        vk::ClearColorValue(std::array<ui32, 4>{ ~0u }),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );

    cmdBuf->end();
    device.executeGraphicsCommandBufferSynchronously(*cmdBuf);

    // Clear atomic counter buffer
    const ui32 MAX_FRAGS = info.maxTransparentFragsPerPixel * info.size.x * info.size.y;
    auto cmdBufT = device.createTransferCommandBuffer();
    cmdBufT->begin(vk::CommandBufferBeginInfo(vk::CommandBufferBeginInfo()));
    cmdBufT->updateBuffer<ui32>(*fragmentListBuffer, 0, { 0, MAX_FRAGS, 0, });
    cmdBufT->end();
    device.executeTransferCommandBufferSyncronously(*cmdBufT);


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
            {}, vk::ImageType::e2D, vk::Format::eR32Uint,
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
                      vk::BufferCopy(sizeof(ui32) * 3, 0, sizeof(ui32)));
}
