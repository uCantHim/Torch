#include "Image.h"

#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include <IL/il.h>

#include "Buffer.h"



vkb::Image::Image(const vk::ImageCreateInfo& info)
{
    recreateImage(info);
}

vkb::Image::Image(const std::string& imagePath, vk::ImageUsageFlags usage)
    : Image()
{
    loadFromFile(imagePath, usage);
}

auto vkb::Image::operator*() const noexcept -> vk::Image
{
    return *image;
}

auto vkb::Image::getMemory() const noexcept -> vk::DeviceMemory
{
    return *memory;
}

auto vkb::Image::getSize() const noexcept -> vk::Extent3D
{
    return extent;
}

auto vkb::Image::getType() const noexcept -> vk::ImageType
{
    return type;
}

auto vkb::Image::getArrayLayerCount() const noexcept -> uint32_t
{
    return arrayLayers;
}

auto vkb::Image::getMipLevelCount() const noexcept -> uint32_t
{
    return mipLevels;
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

void vkb::Image::loadFromFile(const std::string& imagePath, vk::ImageUsageFlags usage)
{
    assert(!imagePath.empty());

    if (!fs::is_regular_file(imagePath)) {
        throw std::runtime_error("In vkb::Image::loadFromFile(): " + imagePath + " is not a file");
    }

    ILuint imageId = ilGenImage();
    ilBindImage(imageId);
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    ILboolean success = ilLoadImage(imagePath.c_str());
    if (!success)
    {
        ilDeleteImage(imageId);
        throw std::runtime_error("Unable to load image from \"" + imagePath + "\":"
                                 + std::to_string(ilGetError()));
    }

    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    uint32_t imageWidth = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_WIDTH));
    uint32_t imageHeight = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_HEIGHT));

    vk::ImageCreateInfo info(
        {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
        { imageWidth, imageHeight, 1 }, 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage);
    recreateImage(info);

    changeLayout(info.initialLayout, vk::ImageLayout::eTransferDstOptimal);
    copyRawData(ilGetData(), 4 * imageWidth * imageHeight, {});
    changeLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);

    ilDeleteImage(imageId);
}

void vkb::Image::copyRawData(uint8_t* data, size_t size, ImageSize copySize)
{
    copySize.extent = expandExtent(copySize.extent);

    assert(data != nullptr);
    // Check for maximum possible per-channel size (which is 64 bit)
    assert(size <= extent.width * extent.height * extent.depth * 4 * sizeof(uint64_t));
    // Check for size
    assert(copySize.extent.width <= extent.width + copySize.offset.x
           && copySize.extent.height <= extent.height + copySize.offset.y
           && copySize.extent.depth <= extent.depth + copySize.offset.z);

    // Create staging buffer
    const auto imageMemReq = getDevice()->getImageMemoryRequirements(*image);
    vkb::Buffer staging(imageMemReq.size, vk::BufferUsageFlagBits::eTransferSrc,
                        vk::MemoryPropertyFlagBits::eHostCoherent
                        | vk::MemoryPropertyFlagBits::eHostVisible);

    std::cout << "Image requires " << imageMemReq.size << " bytes\n";
    std::cout << "Copying " << size << " bytes\n";

    auto buf = staging.map();
    memcpy(buf, data, size);
    staging.unmap();

    // Copy buffer into image
    auto cmdBuf = getDevice().createTransferCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo());
    cmdBuf->copyBufferToImage(
        *staging, *image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy(0, 0, 0, copySize.subres, copySize.offset, copySize.extent));
    cmdBuf->end();
    getDevice().executeTransferCommandBufferSyncronously(*cmdBuf);
}

auto vkb::Image::createView(
    vk::ImageViewType viewType,
    vk::Format viewFormat,
    vk::ComponentMapping componentMapping,
    vk::ImageSubresourceRange subRes) const -> vk::UniqueImageView
{
    return getDevice()->createImageViewUnique(
        { {}, *image, viewType, viewFormat, componentMapping, subRes }
    );
}

void vkb::Image::changeLayout(vk::ImageLayout from, vk::ImageLayout to,
                              vk::ImageSubresourceRange subRes)
{
    auto cmdBuf = getDevice().createGraphicsCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo{});
    changeLayout(*cmdBuf, from, to, subRes);
    cmdBuf->end();
    getDevice().executeGraphicsCommandBufferSynchronously(*cmdBuf);
}

void vkb::Image::changeLayout(vk::CommandBuffer cmdBuf,
                              vk::ImageLayout from, vk::ImageLayout to,
                              vk::ImageSubresourceRange subRes)
{
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlags(),
        {},
        {},
        vk::ImageMemoryBarrier{ {}, {}, from, to, 0, 0, *image, subRes }
    );
}

auto vkb::Image::expandExtent(vk::Extent3D otherExtent) -> vk::Extent3D
{
    return {
        otherExtent.width  == UINT32_MAX ? extent.width  : otherExtent.width,
        otherExtent.height == UINT32_MAX ? extent.height : otherExtent.height,
        otherExtent.depth  == UINT32_MAX ? extent.depth  : otherExtent.depth
    };
}

void vkb::Image::recreateImage(const vk::ImageCreateInfo& info)
{
    auto createInfo = info;
    // Always include the flag that allows views of different formats
    createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
    // Always include the transfer dst bit
    if (!(info.usage & vk::ImageUsageFlagBits::eTransferDst)) {
        createInfo.usage = createInfo.usage | vk::ImageUsageFlagBits::eTransferDst;
    }

    // Create image
    image = getDevice()->createImageUnique(createInfo);

    // Allocate memory for the image
    const auto imageMemReq = getDevice()->getImageMemoryRequirements(*image);
    memory = getDevice()->allocateMemoryUnique(
        vk::MemoryAllocateInfo{
            imageMemReq.size,
            getPhysicalDevice().findMemoryType(
                imageMemReq.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eDeviceLocal)
        }
    );

    getDevice()->bindImageMemory(*image, *memory, 0);

    // Fill helper fields
    type = info.imageType;
    extent = info.extent;
    arrayLayers = info.arrayLayers;
    mipLevels = info.mipLevels;
}
