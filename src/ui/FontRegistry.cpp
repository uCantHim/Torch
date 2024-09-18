#include "trc/ui/FontRegistry.h"

#include <ranges>



auto trc::ui::FontRegistry::addFont(const fs::path& file, ui32 fontSize) -> ui32
{
    const ui32 fontIndex = nextFontIndex++;
    auto& cache = fonts.emplace(fontIndex, new GlyphCache(Face(file, fontSize)));

    onFontAdd(fontIndex, *cache);

    return fontIndex;
}

auto trc::ui::FontRegistry::getFontInfo(ui32 fontIndex) -> const Face&
{
    assert(fonts.at(fontIndex) != nullptr);
    return fonts.at(fontIndex)->getFace();
}

auto trc::ui::FontRegistry::getGlyph(ui32 fontIndex, CharCode character) -> const GlyphMeta&
{
    assert(fonts.at(fontIndex) != nullptr);
    return fonts.at(fontIndex)->getGlyph(character);
}

void trc::ui::FontRegistry::setFontAddCallback(std::function<void(ui32, const GlyphCache&)> func)
{
    onFontAdd = std::move(func);
}

auto trc::ui::FontRegistry::getFonts() -> std::generator<std::pair<ui32, GlyphCache&>>
{
    for (auto [i, glyphs] : std::views::enumerate(fonts))
    {
        if (glyphs != nullptr) {
            co_yield {i, *glyphs};
        }
    }
}
