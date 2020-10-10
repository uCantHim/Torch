#include "Image.h"

#include <IL/il.h>

#include "Buffer.h"



vkb::Image::Image(const vk::ImageCreateInfo& createInfo, const DeviceMemoryAllocator& allocator)
    :
    Image(getDevice(), createInfo, allocator)
{}

vkb::Image::Image(
    const Device& device,
    const vk::ImageCreateInfo& createInfo,
    const DeviceMemoryAllocator& allocator)
{
    createNewImage(device, createInfo, allocator);
}

auto vkb::Image::operator*() const noexcept -> vk::Image
{
    return *image;
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
}

void vkb::Image::writeData(void* srcData, size_t srcSize, ImageSize destArea)
{
    Buffer buf(
        srcSize, srcData,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    auto cmdBuf = getDevice().createGraphicsCommandBuffer();
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
    getDevice().executeGraphicsCommandBufferSynchronously(*cmdBuf);
}

auto vkb::Image::getDefaultSampler() const -> vk::Sampler
{
    if (!defaultSampler)
    {
        defaultSampler = getDevice()->createSamplerUnique(
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

auto vkb::Image::createView(
    vk::ImageViewType viewType,
    vk::Format viewFormat,
    vk::ComponentMapping componentMapping,
    vk::ImageSubresourceRange subRes) const -> vk::UniqueImageView
{
    return getDevice()->createImageViewUnique(
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

    currentLayout = vk::ImageLayout::eUndefined;
    size = createInfo.extent;
}

auto vkb::makeImage2D(
    glm::vec4 color,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& allocator) -> Image
{
    return makeImage2D(getDevice(), color, usage, allocator);
}

auto vkb::makeImage2D(
    const Device& device,
    glm::vec4 color,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& allocator) -> Image
{
    Image result{
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            { 1, 1, 1 }, 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage
        ),
        allocator
    };

    // Why does R8G8B8A8Unorm assume that the supplied data is in signed integer format?
    std::array<uint8_t, 4> normalizedUnsigned{
        static_cast<uint8_t>(color.r * 128.0f),
        static_cast<uint8_t>(color.g * 128.0f),
        static_cast<uint8_t>(color.b * 128.0f),
        static_cast<uint8_t>(color.a * 128.0f),
    };
    result.writeData(normalizedUnsigned.data(), sizeof(uint8_t) * 4, {});

    return result;
}

auto vkb::makeImage2D(
    const fs::path& filePath,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& allocator) -> Image
{
    return makeImage2D(getDevice(), filePath, usage, allocator);
}

auto vkb::makeImage2D(
    const Device& device,
    const fs::path& filePath,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& allocator) -> Image
{
    if (!fs::is_regular_file(filePath)) {
        throw std::runtime_error(filePath.string() + " is not a file");
    }

    ILuint imageId = ilGenImage();
    ilBindImage(imageId);
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
    ILboolean success = ilLoadImage(filePath.c_str());
    if (!success)
    {
        ilDeleteImage(imageId);
        throw std::runtime_error("Unable to load image from \"" + filePath.string() + "\":"
                                 + std::to_string(ilGetError()));
    }
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    const auto imageWidth = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_WIDTH));
    const auto imageHeight = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_HEIGHT));
    Image result{
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            { imageWidth, imageHeight, 1 }, 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage
        ),
        allocator
    };

    result.writeData(ilGetData(), 4 * imageWidth * imageHeight, {});

    return result;
}
