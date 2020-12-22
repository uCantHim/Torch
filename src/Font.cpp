#include "Font.h"



vkb::StaticInit trc::FontDescriptor::_init{
    [] {
        descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo(
                {},
                std::vector<vk::DescriptorSetLayoutBinding>{
                    { 0, vk::DescriptorType::eCombinedImageSampler, 1,
                      vk::ShaderStageFlagBits::eFragment }
                }
            )
        );
    }
};

trc::FontDescriptor::FontDescriptor(GlyphMap& glyphMap)
    :
    imageView(glyphMap.getGlyphImage().createView(vk::ImageViewType::e2D, vk::Format::eR8Unorm)),
    descPool(
        vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            1,
            std::vector<vk::DescriptorPoolSize>{
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
            }
        ))
    ),
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

auto trc::FontDescriptor::getLayout() -> vk::DescriptorSetLayout
{
    return *descLayout;
}

auto trc::FontDescriptor::getProvider() const -> const DescriptorProviderInterface&
{
    return provider;
}



trc::Font::Font(const fs::path& path, ui32 fontSize)
    :
    face(path, fontSize),
    descriptor(glyphMap),
    lineBreakAdvance(static_cast<float>((*face.face)->size->metrics.height >> 6)
                     / static_cast<float>(face.maxGlyphHeight))
{
}

auto trc::Font::getGlyph(CharCode charCode) -> Glyph
{
    auto it = glyphs.find(charCode);
    if (it == glyphs.end())
    {
        GlyphMeta newGlyph = loadGlyphBitmap(*face.face, charCode);

        auto tex = glyphMap.addGlyph(newGlyph);
        it = glyphs.try_emplace(
            charCode,
            Glyph{ tex.lowerLeft, tex.upperRight, newGlyph.size, newGlyph.bearingY, newGlyph.advance }
        ).first;
    }

    return it->second;
}

auto trc::Font::getLineBreakAdvance() const noexcept -> float
{
    return lineBreakAdvance;
}

auto trc::Font::getDescriptor() const -> const FontDescriptor&
{
    return descriptor;
}
