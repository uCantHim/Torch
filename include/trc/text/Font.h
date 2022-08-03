#pragma once

#include <unordered_map>
#include <vector>

#include <vkb/Device.h>
#include <vkb/MemoryPool.h>

#include "trc_util/data/ObjectId.h"
#include "trc/Types.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/core/DescriptorProvider.h"
#include "GlyphMap.h"

namespace trc
{
    class FontRegistry;

    struct Font
    {
        using Registry = FontRegistry;
    };

    template<>
    struct AssetData<Font>
    {
        ui32 fontSize;
        std::vector<std::byte> fontData;
    };

    using FontHandle = AssetHandle<Font>;

    template<>
    auto loadAssetFromFile<Font>(const fs::path& path) -> AssetData<Font>;

    /**
     * @brief Load font data from any font file
     */
    auto loadFont(const fs::path& path, ui32 fontSize) -> AssetData<Font>;

    struct GlyphDrawData
    {
        vec2 texCoordLL; // lower left texture coordinate
        vec2 texCoordUR; // upper right texture coordinate

        vec2 size;

        float bearingY;
        float advance;
    };

    struct FontRegistryCreateInfo
    {
        const vkb::Device& device;
        size_t maxFonts{ 50 };
    };

    /**
     * @brief
     */
    class FontRegistry : public AssetRegistryModuleCacheCrtpBase<Font>
    {
    public:
        explicit FontRegistry(const FontRegistryCreateInfo& createInfo);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) override;

        auto add(u_ptr<AssetSource<Font>> source) -> LocalID override;
        void remove(LocalID id) override;
        auto getHandle(LocalID id) -> AssetHandle<Font> override;

        void load(LocalID id) override;
        void unload(LocalID id) override;

        auto getDescriptorSetLayout() const -> vk::DescriptorSetLayout;

    private:
        friend class AssetHandle<Font>;

        struct GlyphMapDescriptorSet
        {
            vk::UniqueImageView imageView;
            vk::UniqueDescriptorSet set;
        };

        struct FontData
        {
            FontData(FontRegistry& storage, Face face);

            auto getGlyph(CharCode charCode) -> GlyphDrawData;

            Face face;
            GlyphMap* glyphMap;
            DescriptorProvider descProvider;

            // Meta
            float lineBreakAdvance;

            std::unordered_map<CharCode, GlyphDrawData> glyphs;
        };

        struct FontStorage
        {
            u_ptr<AssetSource<Font>> source;
            u_ptr<FontData> font;
            u_ptr<ReferenceCounter> refCounter;
        };

        auto allocateGlyphMap() -> std::pair<GlyphMap*, DescriptorProvider>;
        auto makeDescSet(GlyphMap& map) -> GlyphMapDescriptorSet;

        const vkb::Device& device;
        data::IdPool idPool;

        // Storage GPU resources
        vkb::MemoryPool memoryPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorPool descPool;

        // Managed glyph maps
        std::vector<GlyphMap> glyphMaps;
        std::vector<FontStorage> fonts;
        std::vector<GlyphMapDescriptorSet> glyphMapDescSets;
    };

    template<>
    class AssetHandle<Font>
    {
    public:
        AssetHandle() = delete;

        AssetHandle(const AssetHandle&) = default;
        AssetHandle(AssetHandle&&) noexcept = default;
        AssetHandle& operator=(const AssetHandle&) = default;
        AssetHandle& operator=(AssetHandle&&) noexcept = default;
        ~AssetHandle() = default;

        /**
         * @brief Retrieve information about a glyph from the font
         *
         * Loads previously unused glyphs lazily.
         */
        auto getGlyph(CharCode charCode) -> GlyphDrawData;

        /**
         * @return float The amount of space between vertical lines of text
         */
        auto getLineBreakAdvance() const noexcept -> float;

        auto getDescriptor() const -> const DescriptorProvider&;

    private:
        friend FontRegistry;
        explicit AssetHandle(FontRegistry::FontStorage& storage);

        FontRegistry::SharedCacheReference cacheRef;
        FontRegistry::FontData* data;
    };
} // namespace trc
