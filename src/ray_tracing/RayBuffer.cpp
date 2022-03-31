#include "ray_tracing/RayBuffer.h"

#include "DescriptorSetUtils.h"
#include "ray_tracing/RayPipelineBuilder.h"



trc::rt::RayBuffer::RayBuffer(const vkb::Device& device, const RayBufferCreateInfo& info)
    :
    size(info.size)
{
    // Reflections image
    auto& reflections = images.emplace_back(
        device,
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            { size.x, size.y, 1 }, 1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            info.imageUsage | vk::ImageUsageFlagBits::eStorage
        ),
        info.alloc
    );
    imageViews.emplace_back(reflections.createView());

    createDescriptors(device);
}

auto trc::rt::RayBuffer::getSize() const -> uvec2
{
    return size;
}

auto trc::rt::RayBuffer::getExtent() const -> vk::Extent2D
{
    return { size.x, size.y };
}

auto trc::rt::RayBuffer::getImage(Image imageType) -> vkb::Image&
{
    return images.at(imageType);
}

auto trc::rt::RayBuffer::getImage(Image imageType) const -> const vkb::Image&
{
    return images.at(imageType);
}

auto trc::rt::RayBuffer::getImageView(Image imageType) const -> vk::ImageView
{
    return *imageViews.at(imageType);
}

auto trc::rt::RayBuffer::getImageDescriptor(Image imageType) const -> const DescriptorProvider&
{
    return *singleImageProviders.at(imageType);
}

auto trc::rt::RayBuffer::getImageDescriptorSet(Image imageType) const -> vk::DescriptorSet
{
    return *sets.at(imageType);
}

auto trc::rt::RayBuffer::getImageDescriptorLayout() const -> vk::DescriptorSetLayout
{
    return *layout;
}

void trc::rt::RayBuffer::createDescriptors(const vkb::Device& device)
{
    assert(imageViews.size() == Image::NUM_IMAGES);

    layout = buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1, ALL_RAY_PIPELINE_STAGE_FLAGS)
        .build(device);

    vk::DescriptorPoolSize poolSize{ vk::DescriptorType::eStorageImage, Image::NUM_IMAGES };
    pool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        Image::NUM_IMAGES,  // One descriptor set for each image
        poolSize
    });

    std::vector<vk::DescriptorSetLayout> layouts{ Image::NUM_IMAGES, *layout };
    sets = device->allocateDescriptorSetsUnique({ *pool, layouts });

    // Write sets and create providers
    for (ui32 i = 0; auto& set : sets)
    {
        vk::DescriptorImageInfo imageInfo({}, *imageViews.at(i++), vk::ImageLayout::eGeneral);
        vk::WriteDescriptorSet write(*set, 0, 0, vk::DescriptorType::eStorageImage, imageInfo);
        device->updateDescriptorSets(write, {});

        singleImageProviders.emplace_back(new DescriptorProvider(*layout, *set));
    }
}
