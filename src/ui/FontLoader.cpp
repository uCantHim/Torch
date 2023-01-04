#include "trc/ui/FontLoader.h"

#include "trc/text/UnicodeUtils.h"



namespace trc::ui
{

FontLayouter::FontLayouter(FontLoader& backend)
    : fontLoader(&backend)
{
}

auto FontLayouter::getFontInfo(ui32 fontIndex) -> const Face&
{
    return fontLoader->getFontInfo(fontIndex);
}

auto FontLayouter::getGlyph(ui32 fontIndex, CharCode character) -> const GlyphMeta&
{
    return fontLoader->getGlyph(fontIndex, character);
}

auto FontLayouter::layoutText(const std::string& str, ui32 fontIndex, vec2 scaling)
    -> std::pair<types::Text, vec2>
{
    return layoutText(decodeUtf8(str), fontIndex, scaling);
}

auto FontLayouter::layoutText(const std::vector<CharCode>& chars, ui32 fontIndex, vec2 scaling)
    -> std::pair<types::Text, vec2>
{
    types::Text textInfo{
        .fontIndex = fontIndex,
        .letters = {}
    };

    // Create letter descriptions
    const auto& face = getFontInfo(fontIndex);

    vec2 glyphPos{ 0, face.maxAscendNorm };
    vec2 textSize{ 0, face.maxAscendNorm - face.maxDescendNorm };
    float currentLineWidth{ 0 };

    for (CharCode code : chars)
    {
        if (code == '\n')
        {
            glyphPos.x = 0.0f;
            glyphPos.y += face.lineSpaceNorm;

            // Count full text size
            textSize.y += face.maxLineHeightNorm;
            currentLineWidth = 0;

            continue;
        }

        auto& glyph = getGlyph(fontIndex, code);
        textInfo.letters.emplace_back(types::Text::LetterInfo{
            .characterCode = code,
            .glyphOffset = glyphPos * scaling,
            .glyphSize = glyph.metaNormalized.size * scaling,
            .bearingY = glyph.metaNormalized.bearingY * scaling.y
        });

        glyphPos.x += glyph.metaNormalized.advance;

        // Count text size
        textSize.x = glm::max(textSize.x, currentLineWidth += glyph.metaNormalized.advance);
    }

    // Add null-character at the end
    auto nullChar = face.loadGlyph(0x00);
    textInfo.letters.emplace_back(types::Text::LetterInfo{
        .characterCode = 0x00,
        .glyphOffset = glyphPos * scaling,
        .glyphSize = nullChar.metaNormalized.size * scaling,
        .bearingY = nullChar.metaNormalized.bearingY * scaling.y
    });

    return { textInfo, textSize * scaling };
}

auto FontLayouter::getFontHeight(ui32 fontIndex) -> float
{
    const auto& face = getFontInfo(fontIndex);
    return face.maxAscendNorm - face.maxDescendNorm;
}

auto FontLayouter::getFontHeightPixels(ui32 fontIndex) -> float
{
    const auto& face = getFontInfo(fontIndex);
    return face.maxAscend - face.maxDescend;
}

auto FontLayouter::getFontHeightLatin(ui32 fontIndex) -> float
{
    const auto& face = getFontInfo(fontIndex);
    return face.loadGlyph('A').metaNormalized.bearingY
           + face.loadGlyph('g').metaNormalized.negBearingY;
}

auto FontLayouter::getFontHeightLatinPixels(ui32 fontIndex) -> float
{
    const auto& face = getFontInfo(fontIndex);
    return face.loadGlyph('A').metaInPixels.bearingY
           + face.loadGlyph('g').metaInPixels.negBearingY;
}

} // namespace trc::ui
