#include "text/GlyphMap.h"



trc::GlyphMap::GlyphMap()
    :
    image(
        vkb::getDevice(),
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8Unorm,
            { MAP_WIDTH, MAP_HEIGHT, 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
        ),
        memoryPool->makeAllocator()
    )
{
    image.changeLayout(vkb::getDevice(), vk::ImageLayout::eShaderReadOnlyOptimal);
}

auto trc::GlyphMap::addGlyph(const GlyphMeta& glyph) -> UvRectangle
{
    const auto& [data, size] = glyph.pixelData;
    if (data.empty()) {
        return { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    }

    if (offset.x + size.x > MAP_WIDTH)
    {
        offset.x = 0;
        offset.y += maxHeight;
        offset.y += 1; // A small spacing between glyphs to account for uv-inaccuracy
        maxHeight = 0;
    }
    if (offset.y + size.y > MAP_HEIGHT) {
        throw std::out_of_range("No more space in glyph map!");
    }

    image.writeData(
        data.data(),
        size.x * size.y,
        vkb::ImageSize{ .offset={ offset.x, offset.y, 0 }, .extent={ size.x, size.y, 1 } }
    );

    GLM_CONSTEXPR vec2 mapSize{ MAP_WIDTH, MAP_HEIGHT };
    vec2 ll{ vec2(offset) / mapSize };
    vec2 ur{ (vec2(offset) + vec2(size)) / mapSize };

    maxHeight = glm::max(maxHeight, size.y);
    offset.x += size.x;
    offset.x += 1; // A small spacing between glyphs to account for uv-inaccuracy

    return { ll, ur };
}

auto trc::GlyphMap::getGlyphImage() -> vkb::Image&
{
    return image;
}



vkb::StaticInit trc::GlyphMapDescriptor::_init{
    [] {
        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
            { 0, vk::DescriptorType::eCombinedImageSampler, 1,
              vk::ShaderStageFlagBits::eFragment }
        };
        descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
        );
    },
    [] {
        descLayout.reset();
    }
};

trc::GlyphMapDescriptor::GlyphMapDescriptor(GlyphMap& glyphMap)
    :
    imageView(glyphMap.getGlyphImage().createView(vk::ImageViewType::e2D, vk::Format::eR8Unorm)),
    descPool([] {
        std::vector<vk::DescriptorPoolSize> poolSizes{
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
        };
        return vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes)
        );
    }()),
    descSet([&]() -> vk::UniqueDescriptorSet {
        auto set = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
        )[0]);

        vk::DescriptorImageInfo glyphImage(
            glyphMap.getGlyphImage().getDefaultSampler(),
            *imageView,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
        std::vector<vk::WriteDescriptorSet> writes{
            vk::WriteDescriptorSet(
                *set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &glyphImage
            )
        };

        vkb::getDevice()->updateDescriptorSets(writes, {});

        return set;
    }()),
    provider(*descLayout, *descSet)
{
}

auto trc::GlyphMapDescriptor::getLayout() -> vk::DescriptorSetLayout
{
    return *descLayout;
}

auto trc::GlyphMapDescriptor::getProvider() const -> const DescriptorProviderInterface&
{
    return provider;
}
