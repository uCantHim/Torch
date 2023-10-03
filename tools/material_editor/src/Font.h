#pragma once

#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include <trc/core/Window.h>
#include <trc/text/Font.h>
#include <trc/text/GlyphMap.h>

using namespace trc::basic_types;

class Font;

/**
 * @brief Calculate the size of rendered text
 */
auto calcTextSize(std::string_view str, float scaling, Font& font) -> vec2;

/**
 * @return Font& The material editor's canonical font for natural-language,
 *               descriptive text.
 * @throw std::out_of_range if the font has not been set.
 */
auto getTextFont() -> Font&;

/**
 * @return Font& The material editor's canonical monospaced font. Used for
 *               numeric values or other text with syntax restrictions.
 * @throw std::out_of_range if the font has not been set.
 */
auto getMonoFont() -> Font&;

void setGlobalTextFont(Font font);
void setGlobalMonoFont(Font font);
void destroyGlobalFonts();

struct GlyphData
{
    vec2 size;
    vec2 uvPos;
    vec2 uvSize;
};

class Font : public trc::Face
{
public:
    Font(Font&&) noexcept = default;

    Font(const trc::Device& device, const trc::FontData& data)
        :
        Face(data.fontData, data.fontSize),
        glyphMap(device)
    {}

    auto getMetadata(trc::CharCode c) -> trc::GlyphMeta
    {
        return loadGlyph(c);
    }

    auto getGlyph(trc::CharCode c) -> const GlyphData&
    {
        auto [it, success] = glyphCache.try_emplace(c);
        if (!success) {
            return it->second;
        }

        const auto meta = loadGlyph(c);
        const auto uv = glyphMap.addGlyph(meta);
        it->second = GlyphData{
            .size=meta.metaNormalized.size,
            .uvPos=uv.lowerLeft,
            .uvSize=uv.upperRight - uv.lowerLeft,
        };

        return it->second;
    }

    auto getTexture() const -> const trc::Image& {
        return glyphMap.getGlyphImage();
    }

private:
    trc::GlyphMap glyphMap;
    std::unordered_map<trc::CharCode, GlyphData> glyphCache;
};
