#pragma once

#include <memory>

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <vkb/StaticInit.h>

#include "Types.h"
#include "DescriptorProvider.h"
#include "text/GlyphLoading.h"

namespace trc
{
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

        GlyphMap();

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

    private:
        static constexpr ui32 MAP_WIDTH{ 5000 };
        static constexpr ui32 MAP_HEIGHT{ 1000 };

        static inline std::unique_ptr<vkb::MemoryPool> memoryPool;
        static inline vkb::StaticInit _init{
            [] { memoryPool.reset(new vkb::MemoryPool(vkb::getDevice(), 25000000)); },
            [] { memoryPool.reset(); }
        };

        ivec2 offset{ 0, 0 };
        ui32 maxHeight{ 0 };
        vkb::Image image;
    };

    /**
     * Contains a combined image sampler (format eR8Unorm) at binding 0
     */
    class GlyphMapDescriptor
    {
    public:
        explicit GlyphMapDescriptor(GlyphMap& glyphMap);

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
} // namespace trc
