#include "trc/base/Image.h"

#include "trc/base/Barriers.h"
#include "trc/base/Buffer.h"



trc::Image::Image(
    const trc::Device& device,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& alloc)
    :
    Image(
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            format,
            { width, height, 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            usage
        ),
        alloc
    )
{
}

trc::Image::Image(
    const Device& device,
    const vk::ImageCreateInfo& createInfo,
    const DeviceMemoryAllocator& allocator)
    :
    device(&device)
{
    createNewImage(device, createInfo, allocator);
}

auto trc::Image::operator*() const noexcept -> vk::Image
{
    return *image;
}

auto trc::Image::getType() const noexcept -> vk::ImageType
{
    return type;
}

auto trc::Image::getFormat() const noexcept -> vk::Format
{
    return format;
}

auto trc::Image::getSize() const noexcept -> glm::uvec2
{
    return { extent.width, extent.height };
}

auto trc::Image::getExtent() const noexcept -> vk::Extent3D
{
    return extent;
}

void trc::Image::writeData(
    const void* srcData,
    size_t srcSize,
    const ImageSize& destArea,
    const DeviceMemoryAllocator& alloc)
{
    Buffer buf(
        *device,
        srcSize, srcData,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
        alloc
    );

    device->executeCommands(QueueType::transfer, [&](auto cmdBuf)
    {
        const auto copyExtent = expandExtent(destArea.extent);
        cmdBuf.copyBufferToImage(
            *buf, *image,
            vk::ImageLayout::eTransferDstOptimal,
            vk::BufferImageCopy(0, 0, 0, destArea.subres, destArea.offset, copyExtent)
        );
    });
}

void trc::Image::writeData(
    vk::CommandBuffer cmdBuf,
    vk::Buffer srcData,
    const ImageSize& destArea)
{
    const auto copyExtent = expandExtent(destArea.extent);
    cmdBuf.copyBufferToImage(
        srcData, *image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy(0, 0, 0, destArea.subres, destArea.offset, copyExtent)
    );
}

auto trc::Image::getMemory() const noexcept -> const DeviceMemory&
{
    return memory;
}

auto trc::Image::getDefaultSampler() const -> vk::Sampler
{
    if (!defaultSampler)
    {
        defaultSampler = device->get().createSamplerUnique(
            vk::SamplerCreateInfo(
                vk::SamplerCreateFlags(),
                vk::Filter::eLinear, vk::Filter::eLinear,
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                                             // mip LOD bias
                true, 8.0f,                                       // anisotropy
                false, vk::CompareOp::eNever,                     // depth compare
                0.0f, 0.0f,                                       // min/max LOD
                vk::BorderColor::eFloatOpaqueWhite,
                false                                             // unnormalized coordinates?
            )
        );
    }

    return *defaultSampler;
}

auto trc::Image::createView(vk::ImageAspectFlags aspect) const -> vk::UniqueImageView
{
    return device->get().createImageViewUnique(
        vk::ImageViewCreateInfo(
            {}, *image,
            type == vk::ImageType::e2D ? vk::ImageViewType::e2D
                : type == vk::ImageType::e3D ? vk::ImageViewType::e3D
                : vk::ImageViewType::e1D,
            format,
            {}, // component mapping
            vk::ImageSubresourceRange(aspect, 0, 1, 0, 1)
        )
    );
}

auto trc::Image::createView(
    vk::ImageViewType viewType,
    vk::Format viewFormat,
    vk::ComponentMapping componentMapping,
    vk::ImageSubresourceRange subRes) const -> vk::UniqueImageView
{
    return device->get().createImageViewUnique(
        { {}, *image, viewType, viewFormat, componentMapping, subRes }
    );
}

auto trc::Image::expandExtent(vk::Extent3D otherExtent) -> vk::Extent3D
{
    return {
        otherExtent.width  == UINT32_MAX ? extent.width  : otherExtent.width,
        otherExtent.height == UINT32_MAX ? extent.height : otherExtent.height,
        otherExtent.depth  == UINT32_MAX ? extent.depth  : otherExtent.depth
    };
}

void trc::Image::createNewImage(
    const Device& device,
    const vk::ImageCreateInfo& createInfo,
    const DeviceMemoryAllocator& allocator)
{
    image = device->createImageUnique(createInfo);
    auto memReq = device->getImageMemoryRequirements(*image);
    memory = allocator(device, vk::MemoryPropertyFlagBits::eDeviceLocal, memReq);
    memory.bindToImage(device, *image);

    type = createInfo.imageType;
    format = createInfo.format;
    extent = createInfo.extent;
}
