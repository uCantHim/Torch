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

    /**
     * This struct can be used in the following ways:
     *     - Register it at a `FontRegistry` to create an asset via Torch's
     *       asset system.
     *     - Use the data manually.
     *
     * The following steps can be taken to use fonts manually:
     *     1. Create a `Face` object from the font data:
     *        `Face face{ data.fontData, data.fontSize };`
     *        This object allows you to access individual glyphs of the font. It
     *        also contains meta information about the font.
     *     2. Load the glyph you want to render: `auto g = face.loadGlyph('A');`
     *        Glyphs contains pixel data as well as size information. The latter
     *        is always available in pixel units and normalized float units.
     *        Normalized coordinates don't depend on the loaded font size.
     *     3. Create a `GlyphMap` object: `GlyphMap map{ device };`
     *        This object is an image on the device that allows you to store
     *        glyphs in it.
     *     4. Upload your glyph to the glyph map: `auto pos = map.addGlyph(g);`
     *        The result is a rectangle that specifies the glyph's position and
     *        size in the glyph map. You can derive UV coordinates from this
     *        rectangle.
     */
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

        // The descriptor binding that will contain glyph map images. Must be
        // an array of image samplers.
        SharedDescriptorSet::Binding glyphMapBinding;

        // The font registry allocates glyph maps from a memory pool. This
        // setting controls the size of that pool in bytes.
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
