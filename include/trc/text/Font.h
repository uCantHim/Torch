#pragma once

#include "GlyphMap.h"
#include "DescriptorProvider.h"

namespace trc
{
    struct Glyph
    {
        vec2 texCoordLL; // lower left texture coordinate
        vec2 texCoordUR; // upper right texture coordinate

        vec2 size;

        float bearingY;
        float advance;
    };

    /**
     * Contains a combined image sampler (format eR8Unorm) at binding 0
     */
    class FontDescriptor
    {
    public:
        explicit FontDescriptor(GlyphMap& glyphMap);

        static auto getLayout() -> vk::DescriptorSetLayout;

        auto getProvider() const -> const DescriptorProviderInterface&;

    private:
        static inline vk::UniqueDescriptorSetLayout descLayout;
        static vkb::StaticInit _init;

        vk::UniqueImageView imageView;
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSet descSet;
        DescriptorProvider provider;
    };

    class Font
    {
    public:
        explicit Font(const fs::path& path, ui32 fontSize = 18);

        /**
         * @brief Retrieve information about a glyph from the font
         *
         * Loads previously unused glyphs lazily.
         */
        auto getGlyph(CharCode charCode) -> Glyph;

        /**
         * @return float The amount of space between vertical lines of text
         */
        auto getLineBreakAdvance() const noexcept -> float;

        auto getDescriptor() const -> const FontDescriptor&;

    private:
        Face face;
        GlyphMap glyphMap;
        FontDescriptor descriptor;

        // Meta
        float lineBreakAdvance;

        std::unordered_map<CharCode, Glyph> glyphs;
    };
} // namespace trc
