#pragma once

#include <unordered_map>
#include <vector>

#include <trc_util/data/IdPool.h>
#include "trc/base/Device.h"
#include "trc/base/MemoryPool.h"

#include "trc/Types.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/assets/SharedDescriptorSet.h"
#include "trc/core/DescriptorProvider.h"
#include "trc/text/GlyphMap.h"

namespace trc
{
    class FontRegistry;

    struct Font
    {
        using Registry = FontRegistry;

        static consteval auto name() -> std::string_view {
            return "torch_font";
        }
    };

    template<>
    struct AssetData<Font>
    {
        ui32 fontSize;
        std::vector<std::byte> fontData;

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);
    };

    using FontHandle = AssetHandle<Font>;

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
        const Device& device;
        SharedDescriptorSet::Binding glyphMapBinding;

        size_t glyphMapMemoryPoolSize{ 20000000 };
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

    private:
        friend class AssetHandle<Font>;

        struct FontDeviceData
        {
            GlyphMap glyphMap;
            vk::UniqueImageView glyphImageView;
            ui32 descriptorIndex;
        };

        struct FontData
        {
            FontData(Face face, u_ptr<FontDeviceData> deviceData);

            auto getGlyph(CharCode charCode) -> GlyphDrawData;

            Face face;
            u_ptr<FontDeviceData> deviceData;

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

        const Device& device;
        data::IdPool<ui64> idPool;

        // Storage GPU resources
        MemoryPool memoryPool;
        SharedDescriptorSet::Binding descBinding;

        // Managed glyph maps
        std::vector<FontStorage> fonts;
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

        /**
         * @return ui32 An index that specifies the font's glyph map image
         *              in the glyph map descriptor.
         */
        auto getDescriptorIndex() const -> ui32;

    private:
        friend FontRegistry;
        explicit AssetHandle(FontRegistry::FontStorage& storage);

        FontRegistry::SharedCacheReference cacheRef;
        FontRegistry::FontData* data;
    };
} // namespace trc
