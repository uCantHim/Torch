#pragma once

#include <unordered_map>
#include <vector>

#include <trc_util/data/IdPool.h>

#include "trc/Types.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/assets/DeviceDataCache.h"
#include "trc/assets/SharedDescriptorSet.h"
#include "trc/base/Device.h"
#include "trc/base/MemoryPool.h"
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
    using FontData = AssetData<Font>;

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
    class FontRegistry : public AssetRegistryModuleInterface<Font>
    {
    public:
        explicit FontRegistry(const FontRegistryCreateInfo& createInfo);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) override;

        auto add(u_ptr<AssetSource<Font>> source) -> LocalID override;
        void remove(LocalID id) override;
        auto getHandle(LocalID id) -> AssetHandle<Font> override;

    private:
        friend class AssetHandle<Font>;

        struct DeviceData
        {
            DeviceData(Face face, ui32 deviceIndex, GlyphMap glyphMap);

            auto getGlyph(CharCode charCode) -> GlyphDrawData;

            ui32 descriptorIndex;

            GlyphMap glyphMap;
            vk::UniqueImageView glyphImageView;

            Face face;
            float lineBreakAdvance;
            std::unordered_map<CharCode, GlyphDrawData> glyphs;
        };

        auto loadDeviceData(LocalID id) -> DeviceData;
        void freeDeviceData(LocalID id, DeviceData data);

        const Device& device;
        data::IdPool<ui64> idPool;

        MemoryPool memoryPool;
        SharedDescriptorSet::Binding descBinding;

        std::vector<u_ptr<AssetSource<Font>>> fonts;
        DeviceDataCache<DeviceData> deviceDataStorage;
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
        using DataHandle = DeviceDataCache<FontRegistry::DeviceData>::CacheEntryHandle;

        explicit AssetHandle(DataHandle handle) : data(handle) {}

        DataHandle data;
    };
} // namespace trc
