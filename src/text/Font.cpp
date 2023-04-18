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



auto loadFont(const fs::path& path, ui32 fontSize) -> AssetData<Font>
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to read from file " + path.string());
    }

    std::stringstream ss;
    ss << file.rdbuf();
    auto data = ss.str();

    std::vector<std::byte> _data(data.size());
    memcpy(_data.data(), data.data(), data.size());

    return { .fontSize=fontSize, .fontData=std::move(_data) };
}



AssetHandle<Font>::AssetHandle(FontRegistry::FontStorage& storage)
    :
    cacheRef(*storage.refCounter),
    data(storage.font.get())
{
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
    return data->deviceData->descriptorIndex;
}



trc::FontRegistry::FontRegistry(const FontRegistryCreateInfo& info)
    :
    device(info.device),
    memoryPool(device, info.glyphMapMemoryPoolSize),
    descBinding(info.glyphMapBinding)
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
    fonts.at(id).source = std::move(source);
    fonts.at(id).refCounter = std::make_unique<ReferenceCounter>(id, this);

    return id;
}

void FontRegistry::remove(LocalID id)
{
    fonts.at(id).source.reset();
    fonts.at(id).refCounter.reset();
    fonts.at(id).font.reset();
    idPool.free(id);
}

auto FontRegistry::getHandle(LocalID id) -> AssetHandle<Font>
{
    return AssetHandle<Font>(fonts.at(id));
}

void FontRegistry::load(LocalID id)
{
    auto& storage = fonts.at(id);
    assert(storage.source != nullptr);

    if (storage.font == nullptr)
    {
        // Create a glyph map
        auto deviceData = std::make_unique<FontDeviceData>(FontDeviceData{
            .glyphMap{ device, memoryPool.makeAllocator() },
            .descriptorIndex=static_cast<ui32>(id)
        });
        deviceData->glyphImageView = deviceData->glyphMap.getGlyphImage().createView();

        // Add the new glyph map to the descriptor
        descBinding.update(
            deviceData->descriptorIndex,
            vk::DescriptorImageInfo{
                deviceData->glyphMap.getGlyphImage().getDefaultSampler(),
                *deviceData->glyphImageView,
                vk::ImageLayout::eShaderReadOnlyOptimal,
            }
        );

        // Load host data and store the font
        auto hostData = storage.source->load();
        storage.font = std::make_unique<FontData>(
            Face(std::move(hostData.fontData), hostData.fontSize),
            std::move(deviceData)
        );
    }
}

void FontRegistry::unload(LocalID id)
{
    fonts.at(id).font.reset();
}



FontRegistry::FontData::FontData(Face _face, u_ptr<FontDeviceData> _deviceData)
    :
    face(std::move(_face)),
    deviceData(std::move(_deviceData)),
    lineBreakAdvance(static_cast<float>(face.lineSpace) / static_cast<float>(face.maxGlyphHeight))
{
}

auto FontRegistry::FontData::getGlyph(CharCode charCode) -> GlyphDrawData
{
    auto it = glyphs.find(charCode);
    if (it == glyphs.end())
    {
        GlyphMeta newGlyph = face.loadGlyph(charCode);
        auto tex = deviceData->glyphMap.addGlyph(newGlyph);

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

} // namespace trc
