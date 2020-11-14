#include "PostProcessing.h"



trc::MorpholocialAntiAliasingDescriptor::MorpholocialAntiAliasingDescriptor(
    vk::Extent2D framebufferSize,
    std::vector<std::pair<vk::Sampler, vk::ImageView>> depthImages)
{
    createResources(framebufferSize);
    createDescriptors(std::move(depthImages));
}

auto trc::MorpholocialAntiAliasingDescriptor::getDescriptorSet() const noexcept
    -> vk::DescriptorSet
{
    return **descSets;
}

auto trc::MorpholocialAntiAliasingDescriptor::getDescriptorSetLayout() const noexcept
    -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::MorpholocialAntiAliasingDescriptor::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    cmdBuf.bindDescriptorSets(bindPoint, pipelineLayout, setIndex, **descSets, {});
}

void trc::MorpholocialAntiAliasingDescriptor::createResources(vk::Extent2D framebufferSize)
{
    precomputedAreaTexture = vkb::makeImage2D(TRC_SHADER_DIR"/area_tex_9.dds");
    precomputedAreaTextureView = precomputedAreaTexture.createView(
        vk::ImageViewType::e2D,
        vk::Format::eR8G8B8A8Unorm
    );

    edgeImages = { [framebufferSize](ui32) -> vkb::Image {
        vkb::Image result(vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8Unorm,
            vk::Extent3D{ framebufferSize, 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        ));
        result.changeLayout(vkb::getDevice(), vk::ImageLayout::eGeneral);
        return result;
    }};
    edgeImageViews = { [this](ui32 imageIndex) {
        return edgeImages.getAt(imageIndex).createView(
            vk::ImageViewType::e2D, vk::Format::eR8G8Unorm
        );
    }};

    areaImages = { [framebufferSize](ui32) -> vkb::Image {
        vkb::Image result(vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR32G32B32A32Sfloat,
            vk::Extent3D{ framebufferSize, 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        ));
        result.changeLayout(vkb::getDevice(), vk::ImageLayout::eGeneral);
        return result;
    }};
    areaImageViews = { [this](ui32 imageIndex) {
        return areaImages.getAt(imageIndex).createView(
            vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat
        );
    }};
}

void trc::MorpholocialAntiAliasingDescriptor::createDescriptors(
    std::vector<std::pair<vk::Sampler, vk::ImageView>> depthImages)
{
    descPool = vkb::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            depthImages.size(), // Equivalent to frame count
            std::vector<vk::DescriptorPoolSize>{
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 3),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 3),
            }
        )
    );

    descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo(
            {},
            std::vector<vk::DescriptorSetLayoutBinding>{
                { 0, vk::DescriptorType::eStorageImage,         1, vk::ShaderStageFlagBits::eCompute },
                { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
                { 2, vk::DescriptorType::eStorageImage,         1, vk::ShaderStageFlagBits::eCompute },
                { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
                // Blend weights image
                { 4, vk::DescriptorType::eStorageImage,         1, vk::ShaderStageFlagBits::eCompute },
                // Precomputed area texture
                { 5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute },
            }
        )
    );

    descSets = { [this, &depthImages](ui32 imageIndex) -> vk::UniqueDescriptorSet {
        auto descSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
            { *descPool, *descLayout }
        )[0]);

        vk::DescriptorImageInfo depthImage(
            depthImages.at(imageIndex).first,
            depthImages.at(imageIndex).second,
            vk::ImageLayout::eGeneral
        );
        vk::DescriptorImageInfo edgeImage(
            edgeImages.getAt(imageIndex).getDefaultSampler(),
            *edgeImageViews.getAt(imageIndex),
            vk::ImageLayout::eGeneral
        );
        vk::DescriptorImageInfo areaImage(
            areaImages.getAt(imageIndex).getDefaultSampler(),
            *areaImageViews.getAt(imageIndex),
            vk::ImageLayout::eGeneral
        );
        vk::DescriptorImageInfo precomputedAreaImage(
            precomputedAreaTexture.getDefaultSampler(),
            *precomputedAreaTextureView,
            vk::ImageLayout::eGeneral
        );

        vkb::getDevice()->updateDescriptorSets(
            {
                // Color storage image and sampler
                vk::WriteDescriptorSet(
                    *descSet, 0, 0, 1,
                    vk::DescriptorType::eStorageImage, &depthImage
                ),
                vk::WriteDescriptorSet(
                    *descSet, 1, 0, 1,
                    vk::DescriptorType::eCombinedImageSampler, &depthImage
                ),
                // Edge detection storage image and sampler
                vk::WriteDescriptorSet(
                    *descSet, 2, 0, 1,
                    vk::DescriptorType::eStorageImage, &edgeImage
                ),
                vk::WriteDescriptorSet(
                    *descSet, 3, 0, 1,
                    vk::DescriptorType::eCombinedImageSampler, &edgeImage
                ),
                // Calculated areas storage image
                vk::WriteDescriptorSet(
                    *descSet, 4, 0, 1,
                    vk::DescriptorType::eStorageImage, &areaImage
                ),
                // Pre-computed areas sampler
                vk::WriteDescriptorSet(
                    *descSet, 5, 0, 1,
                    vk::DescriptorType::eCombinedImageSampler, &precomputedAreaImage
                ),
            },
            {}
        );

        return descSet;
    }};
}
