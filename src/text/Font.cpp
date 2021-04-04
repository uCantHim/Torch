#include "text/Font.h"



trc::Font::Font(const fs::path& path, ui32 fontSize)
    :
    face(path, fontSize),
    descriptor(glyphMap),
    lineBreakAdvance(static_cast<float>(face.lineSpace) / static_cast<float>(face.maxGlyphHeight))
{
}

auto trc::Font::getGlyph(CharCode charCode) -> GlyphDrawData
{
    auto it = glyphs.find(charCode);
    if (it == glyphs.end())
    {
        GlyphMeta newGlyph = face.loadGlyph(charCode);

        auto tex = glyphMap.addGlyph(newGlyph);
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

auto trc::Font::getDescriptor() const -> const GlyphMapDescriptor&
{
    return descriptor;
}
