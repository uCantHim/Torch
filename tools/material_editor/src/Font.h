#pragma once

#include <trc/core/Window.h>
#include <trc/text/Font.h>
#include <trc/text/GlyphMap.h>

using namespace trc::basic_types;

struct GlyphData
{
    vec2 size;
    vec2 uvPos;
    vec2 uvSize;
};

class Font : public trc::Face
{
public:
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
