#include "trc/text/Font.h"

#include "font.pb.h"
#include "trc/core/Instance.h"
#include "trc/AssetDescriptor.h"



namespace trc
{

void AssetData<Font>::serialize(std::ostream& os) const
{
    serial::Font font;
    font.set_font_size(fontSize);
    font.set_font_data(fontData.data(), fontData.size());

    font.SerializeToOstream(&os);
}

void AssetData<Font>::deserialize(std::istream& is)
{
    serial::Font data;
    [[maybe_unused]]
    const bool success = data.ParseFromIstream(&is);
    assert(success && "Font data is corrupted. This is probably an engine bug.");

    fontData.resize(data.font_data().size());
    memcpy(fontData.data(), data.font_data().data(), data.font_data().size());
    fontSize = data.font_size();
}



trc::FontRegistry::FontRegistry(const FontRegistryCreateInfo& info)
    :
    device(info.device),
    memoryPool(device, info.glyphMapMemoryPoolSize),
    descBinding(info.glyphMapBinding),
    deviceDataStorage(DeviceDataCache<DeviceData>::makeLoader(
        [this](ui32 id){ return loadDeviceData(LocalID{ id }); },
        [this](ui32 id, DeviceData data){ freeDeviceData(LocalID{ id }, std::move(data)); }
    ))
{
}

void FontRegistry::update(vk::CommandBuffer, FrameRenderState&)
{
}

auto FontRegistry::add(u_ptr<AssetSource<Font>> source) -> LocalID
{
    const LocalID id(idPool.generate());

    if (fonts.size() <= id) {
        fonts.resize(id + 1);
    }
    fonts.at(id) = std::move(source);

    return id;
}

void FontRegistry::remove(LocalID id)
{
    fonts.at(id).reset();
    idPool.free(id);
}

auto FontRegistry::getHandle(LocalID id) -> AssetHandle<Font>
{
    return AssetHandle<Font>(deviceDataStorage.get(id));
}

auto FontRegistry::loadDeviceData(LocalID id) -> DeviceData
{
    assert(fonts.size() > id);
    assert(fonts.at(id) != nullptr);

    auto data = fonts.at(id)->load();

    // Create a glyph map
    DeviceData deviceData{
        Face{ std::move(data.fontData), data.fontSize },
        static_cast<ui32>(id),
        GlyphMap{ device, memoryPool.makeAllocator() },
    };

    // Add the new glyph map to the descriptor
    descBinding.update(
        deviceData.descriptorIndex,
        vk::DescriptorImageInfo{
            deviceData.glyphMap.getGlyphImage().getDefaultSampler(),
            *deviceData.glyphImageView,
            vk::ImageLayout::eShaderReadOnlyOptimal,
        }
    );

    return deviceData;
}

void FontRegistry::freeDeviceData(LocalID /*id*/, DeviceData /*data*/)
{
}



FontRegistry::DeviceData::DeviceData(Face _face, ui32 deviceIndex, GlyphMap _glyphMap)
    :
    // Device data
    descriptorIndex(deviceIndex),
    glyphMap(std::move(_glyphMap)),
    glyphImageView(glyphMap.getGlyphImage().createView()),

    // Host data
    face(std::move(_face)),
    lineBreakAdvance(static_cast<float>(face.lineSpace) / static_cast<float>(face.maxGlyphHeight))
{
}

auto FontRegistry::DeviceData::getGlyph(CharCode charCode) -> GlyphDrawData
{
    auto it = glyphs.find(charCode);
    if (it == glyphs.end())
    {
        GlyphMeta newGlyph = face.loadGlyph(charCode);
        auto tex = glyphMap.addGlyph(newGlyph);

        /**
         * Torch flips the y-axis with the projection matrix. The glyph data,
         * however, is calculated with the text origin in the upper-left corner.
         */
#ifdef TRC_FLIP_Y_PROJECTION
        std::swap(tex.lowerLeft.y, tex.upperRight.y);
        newGlyph.metaNormalized.bearingY = newGlyph.metaNormalized.size.y
                                           - newGlyph.metaNormalized.bearingY;
#endif

        it = glyphs.try_emplace(
            charCode,
            GlyphDrawData{
                .texCoordLL = tex.lowerLeft,
                .texCoordUR = tex.upperRight,
                .size       = newGlyph.metaNormalized.size,
                .bearingY   = newGlyph.metaNormalized.bearingY,
                .advance    = newGlyph.metaNormalized.advance
            }
        ).first;
    }

    return it->second;
}



auto AssetHandle<Font>::getGlyph(CharCode charCode) -> GlyphDrawData
{
    return data->getGlyph(charCode);
}

auto AssetHandle<Font>::getLineBreakAdvance() const noexcept -> float
{
    return data->lineBreakAdvance;
}

auto AssetHandle<Font>::getDescriptorIndex() const -> ui32
{
    return data->descriptorIndex;
}

} // namespace trc
