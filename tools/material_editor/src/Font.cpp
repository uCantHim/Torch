#include "Font.h"

#include <optional>



u_ptr<Font> globalTextFont;
u_ptr<Font> globalMonoFont;

auto getTextFont() -> Font&
{
    if (globalTextFont) {
        return *globalTextFont;
    }

    throw std::out_of_range("[In getTextFont]: No global text font has been defined. Call"
                            " `setDefaultTextFont` before creating text elements!");
}

auto getMonoFont() -> Font&
{
    if (globalMonoFont) {
        return *globalMonoFont;
    }

    throw std::out_of_range("[In getMonoFont]: No global monospaced font has been defined. Call"
                            " `setDefaultMonoFont` before creating text elements!");
}

void setGlobalTextFont(Font font)
{
    globalTextFont = std::make_unique<Font>(std::move(font));
}

void setGlobalMonoFont(Font font)
{
    globalMonoFont = std::make_unique<Font>(std::move(font));
}

void destroyGlobalFonts()
{
    globalTextFont.reset();
    globalMonoFont.reset();
}

auto calcTextSize(std::string_view str, float scaling, Font& font) -> vec2
{
    const float maxGlyphHeight = font.maxAscendNorm + glm::abs(font.maxDescendNorm);

    vec2 size{ 0.0f, scaling * maxGlyphHeight };
    for (char c : str)
    {
        const auto& meta = font.getMetadata(c).metaNormalized;
        size.x += meta.advance * scaling;
    }

    return size;
}
