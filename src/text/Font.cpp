#include "text/Font.h"

#include "text/FontDataStorage.h"



trc::Font::Font(FontDataStorage& storage, Face _face)
    :
    face(std::move(_face)),
    descProvider({}, {}),
    lineBreakAdvance(static_cast<float>(face.lineSpace) / static_cast<float>(face.maxGlyphHeight))
{
    auto [map, provider] = storage.allocateGlyphMap();
    glyphMap = map;
    descProvider = provider;
}

trc::Font::Font(FontDataStorage& storage, const fs::path& path, ui32 fontSize)
    : Font(storage, Face(path, fontSize))
{
}

auto trc::Font::getGlyph(CharCode charCode) -> GlyphDrawData
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

auto trc::Font::getLineBreakAdvance() const noexcept -> float
{
    return lineBreakAdvance;
}

auto trc::Font::getDescriptor() const -> const DescriptorProvider&
{
    return descProvider;
}
