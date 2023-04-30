#pragma once

#include <memory>

#include "trc/base/Image.h"

#include "trc/Types.h"
#include "trc/text/GlyphLoading.h"

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

        GlyphMap(const Device& device,
                 const DeviceMemoryAllocator& alloc = DefaultDeviceMemoryAllocator{});

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
         * @return Image& The image that contains all glyphs
         */
        auto getGlyphImage() -> Image&;

        /**
         * @return Image& The image that contains all glyphs
         */
        auto getGlyphImage() const -> const Image&;

    private:
        static constexpr ui32 MAP_WIDTH{ 5000 };
        static constexpr ui32 MAP_HEIGHT{ 1000 };

        void writeDataToImage(const std::vector<ui8>& data, const ImageSize& dstArea);

        const Device& device;

        ivec2 offset{ 0, 0 };
        ui32 maxHeight{ 0 };
        Image image;
    };
} // namespace trc
