#pragma once

#include <memory>

#include <vkb/Image.h>

#include "../Types.h"
#include "GlyphLoading.h"

namespace trc
{
    class Instance;

    /**
     * @brief An image wrapper that can insert glyph images
     */
    class GlyphMap
    {
    public:
        struct UvRectangle
        {
            vec2 lowerLeft;
            vec2 upperRight;
        };

        GlyphMap(const vkb::Device& device,
                 const vkb::DeviceMemoryAllocator& alloc = vkb::DefaultDeviceMemoryAllocator{});

        /**
         * @param const GlyphMeta& glyph A new glyph
         *
         * @return UvRectangle UV coordinates of the new glyph in the map
         *
         * @throw std::out_of_range if there's no more space in the glyph
         *        texture
         */
        auto addGlyph(const GlyphMeta& glyph) -> UvRectangle;

        /**
         * @return vkb::Image& The image that contains all glyphs
         */
        auto getGlyphImage() -> vkb::Image&;

        /**
         * @return vkb::Image& The image that contains all glyphs
         */
        auto getGlyphImage() const -> const vkb::Image&;

    private:
        static constexpr ui32 MAP_WIDTH{ 5000 };
        static constexpr ui32 MAP_HEIGHT{ 1000 };

        ivec2 offset{ 0, 0 };
        ui32 maxHeight{ 0 };
        vkb::Image image;
    };
} // namespace trc
