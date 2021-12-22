#pragma once

#include <vkb/MemoryPool.h>

#include "../Types.h"
#include "Font.h"

namespace trc
{
    class Instance;

    /**
     * @brief
     */
    class FontDataStorage
    {
    public:
        static constexpr ui32 MAX_FONTS = 200;

        /**
         * @brief
         *
         * @param const Instance& instance
         */
        explicit FontDataStorage(const Instance& instance);

        auto allocateGlyphMap() -> std::pair<GlyphMap*, DescriptorProvider>;
        auto makeFont(const fs::path& filePath, ui32 fontSize = 18) -> Font;

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;

    private:
        struct GlyphMapDescriptorSet
        {
            vk::UniqueImageView imageView;
            vk::UniqueDescriptorSet set;
        };

        auto makeDescSet(GlyphMap& map) -> GlyphMapDescriptorSet;

        const Instance& instance;

        // Storage GPU resources
        vkb::MemoryPool memoryPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;

        // Managed glyph maps
        std::vector<GlyphMap> glyphMaps;
        std::vector<GlyphMapDescriptorSet> glyphMapDescSets;
    };
} // namespace trc
