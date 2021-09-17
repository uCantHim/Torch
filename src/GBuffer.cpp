#include "GBuffer.h"

#include "DescriptorSetUtils.h"



trc::GBuffer::GBuffer(const vkb::Device& device, const GBufferCreateInfo& info)
    :
    size(info.size),
    extent(size.x, size.y)
{
    images.reserve(Image::NUM_IMAGES);
    const auto colorUsage = vk::ImageUsageFlagBits::eColorAttachment
                            | vk::ImageUsageFlagBits::eInputAttachment
                            | vk::ImageUsageFlagBits::eStorage;
    const auto depthUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment
                            | vk::ImageUsageFlagBits::eTransferSrc
                            | vk::ImageUsageFlagBits::eInputAttachment;

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
