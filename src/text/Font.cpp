#include "text/Font.h"

#include "font.pb.h"
#include "core/Instance.h"



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



FontRegistry::FontData::FontData(FontRegistry& storage, Face _face)
    :
    face(std::move(_face)),
    descProvider({}, {}),
    lineBreakAdvance(static_cast<float>(face.lineSpace) / static_cast<float>(face.maxGlyphHeight))
{
    std::tie(glyphMap, descProvider) = storage.allocateGlyphMap();
}

auto FontRegistry::FontData::getGlyph(CharCode charCode) -> GlyphDrawData
{
    auto it = glyphs.find(charCode);
    if (it == glyphs.end())
    {
        GlyphMeta newGlyph = face.loadGlyph(charCode);

        auto tex = glyphMap->addGlyph(newGlyph);
        it = glyphs.try_emplace(
            charCode,
            GlyphDrawData{
                tex.lowerLeft, tex.upperRight,
                newGlyph.metaNormalized.size,
                newGlyph.metaNormalized.bearingY, newGlyph.metaNormalized.advance
            }
        ).first;
    }

    return it->second;
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

auto AssetHandle<Font>::getDescriptor() const -> const DescriptorProvider&
{
    return data->descProvider;
}



trc::FontRegistry::FontRegistry(const FontRegistryCreateInfo& info)
    :
    device(info.device),
    memoryPool(device, 50000000)
{
    // Create descriptor set layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        { 0, vk::DescriptorType::eCombinedImageSampler, 1,
          vk::ShaderStageFlagBits::eFragment }
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
    };
    descPool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes
    });
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
    fonts.at(id).refCounter.reset(new ReferenceCounter(id, this));

    return id;
}

void FontRegistry::remove(LocalID id)
{
    fonts.at(id).source.reset();
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
        auto data = storage.source->load();
        storage.font.reset(new FontData(*this, Face(std::move(data.fontData), data.fontSize)));
    }
}

void FontRegistry::unload(LocalID id)
{
    fonts.at(id).font.reset();
}

auto trc::FontRegistry::allocateGlyphMap() -> std::pair<GlyphMap*, DescriptorProvider>
{
    auto& map = glyphMaps.emplace_back(device, memoryPool.makeAllocator());
    auto& set = glyphMapDescSets.emplace_back(makeDescSet(map));

    return { &map, DescriptorProvider{ *descLayout, *set.set } };
}

auto trc::FontRegistry::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *descLayout;
}

auto trc::FontRegistry::makeDescSet(GlyphMap& glyphMap) -> GlyphMapDescriptorSet
{
    auto imageView = glyphMap.getGlyphImage().createView();
    auto set = std::move(device->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    vk::DescriptorImageInfo glyphImageInfo(
        glyphMap.getGlyphImage().getDefaultSampler(),
        *imageView,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );
    std::vector<vk::WriteDescriptorSet> writes{
        { *set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &glyphImageInfo },
    };

    device->updateDescriptorSets(writes, {});

    return {
        std::move(imageView),
        std::move(set)
    };
}

} // namespace trc
