#include "ui/Font.h"



auto trc::ui::FontRegistry::addFont(const fs::path& file, ui32 fontSize) -> ui32
{
    fonts.emplace(nextFontIndex, new FontData{
        .face = Face(file, fontSize),
        .index = nextFontIndex,
        .glyphs = {}
    });
    nextFontIndex++;

    return nextFontIndex - 1;
}

auto trc::ui::FontRegistry::getFontInfo(ui32 fontIndex) -> const Face&
{
    assert(fonts.at(fontIndex) != nullptr);
    return fonts.at(fontIndex)->face;
}

auto trc::ui::FontRegistry::getGlyph(ui32 fontIndex, wchar_t character) -> const GlyphMeta&
{
    assert(fonts.at(fontIndex) != nullptr);
    FontData& font = *fonts.at(fontIndex);

    auto& glyph = font.glyphs[character];
    if (!glyph) {
        glyph = std::make_unique<GlyphMeta>(loadGlyphBitmap(*font.face.face, character));
    }

    return *glyph;
}
