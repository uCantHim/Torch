#include "Image.h"

#include "Buffer.h"



vkb::Image::Image(const vk::ImageCreateInfo& createInfo, const DeviceMemoryAllocator& allocator)
    :
    Image(getDevice(), createInfo, allocator)
{}

vkb::Image::Image(
    const Device& device,
    const vk::ImageCreateInfo& createInfo,
    const DeviceMemoryAllocator& allocator)
    :
    device(&device)
{
    createNewImage(device, createInfo, allocator);
}

auto vkb::Image::operator*() const noexcept -> vk::Image
{
    return *image;
}

auto vkb::Image::getType() const noexcept -> vk::ImageType
{
    return type;
}

auto vkb::Image::getFormat() const noexcept -> vk::Format
{
    return format;
}

auto vkb::Image::getSize() const noexcept -> vk::Extent3D
{
    return size;
}

void vkb::Image::changeLayout(
    const Device& device,
    vk::ImageLayout newLayout,
    vk::ImageSubresourceRange subRes)
{
    changeLayout(device, currentLayout, newLayout, subRes);
}

void vkb::Image::changeLayout(
    vk::CommandBuffer cmdBuf,
    vk::ImageLayout newLayout,
    vk::ImageSubresourceRange subRes)
{
    changeLayout(cmdBuf, currentLayout, newLayout, subRes);
}

void vkb::Image::changeLayout(
    const Device& device,
    vk::ImageLayout from, vk::ImageLayout to,
    vk::ImageSubresourceRange subRes)
{
    auto cmdBuf = device.createGraphicsCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo());
    changeLayout(*cmdBuf, from, to, subRes);
    cmdBuf->end();
    device.executeGraphicsCommandBufferSynchronously(*cmdBuf);
}

void vkb::Image::changeLayout(
    vk::CommandBuffer cmdBuf,
    vk::ImageLayout from, vk::ImageLayout to,
    vk::ImageSubresourceRange subRes)
{
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlags(),
        {},
        {},
        vk::ImageMemoryBarrier(
            {}, {}, from, to,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *image, std::move(subRes)
        )
    );

    currentLayout = to;
}

void vkb::Image::writeData(const void* srcData, size_t srcSize, ImageSize destArea)
{
    Buffer buf(
        srcSize, srcData,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    auto cmdBuf = device->createGraphicsCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo());

    const auto prevLayout = currentLayout == vk::ImageLayout::eUndefined
                            || currentLayout == vk::ImageLayout::ePreinitialized
                            ? vk::ImageLayout::eGeneral
                            : currentLayout;
    changeLayout(*cmdBuf, vk::ImageLayout::eTransferDstOptimal);
    const auto copyExtent = expandExtent(destArea.extent);
    cmdBuf->copyBufferToImage(
        *buf, *image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy(0, 0, 0, destArea.subres, destArea.offset, copyExtent)
    );
    changeLayout(*cmdBuf, prevLayout);

    cmdBuf->end();
    device->executeGraphicsCommandBufferSynchronously(*cmdBuf);
}

auto vkb::Image::getMemory() const noexcept -> const DeviceMemory&
{
    return memory;
}

auto vkb::Image::getDefaultSampler() const -> vk::Sampler
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
                vk::SamplerAddressMode::eRepeat
            )
        );
    }

    return *defaultSampler;
}

auto vkb::Image::createView(vk::ImageAspectFlags aspect) const -> vk::UniqueImageView
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

auto vkb::Image::createView(
    vk::ImageViewType viewType,
    vk::Format viewFormat,
    vk::ComponentMapping componentMapping,
    vk::ImageSubresourceRange subRes) const -> vk::UniqueImageView
{
    return device->get().createImageViewUnique(
        { {}, *image, viewType, viewFormat, componentMapping, std::move(subRes) }
    );
}

auto vkb::Image::expandExtent(vk::Extent3D otherExtent) -> vk::Extent3D
{
    return {
        otherExtent.width  == UINT32_MAX ? size.width  : otherExtent.width,
        otherExtent.height == UINT32_MAX ? size.height : otherExtent.height,
        otherExtent.depth  == UINT32_MAX ? size.depth  : otherExtent.depth
    };
}

void vkb::Image::createNewImage(
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
    currentLayout = vk::ImageLayout::eUndefined;
    size = createInfo.extent;
}
