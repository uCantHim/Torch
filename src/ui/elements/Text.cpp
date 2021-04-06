#include "ui/elements/Text.h"

#include <iostream>

#include "ui/DrawInfo.h"
#include "text/UnicodeUtils.h"



auto trc::ui::layoutText(const std::string& str, ::trc::ui32 fontIndex)
    -> std::pair<types::Text, ::trc::ivec2>
{
    types::Text textInfo{
        .fontIndex = fontIndex,
        .letters = {}
    };

    // Create letter descriptions
    const auto& face = FontRegistry::getFontInfo(fontIndex);
    const i32 lineHeight{ static_cast<i32>(face.maxAscend) - face.maxDescend };
    ivec2 glyphPos{ 0, face.maxAscend };
    ivec2 textSize{ 0, lineHeight };
    i32 currentLineWidth{ 0 };

    iterUtf8(str, [&](CharCode code) {
        if (code == '\n')
        {
            glyphPos.x = 0.0f;
            glyphPos.y += face.lineSpace;

            // Count full text size
            textSize.y += lineHeight;
            currentLineWidth = 0;

            return;
        }

        auto& glyph = FontRegistry::getGlyph(fontIndex, code);
        textInfo.letters.emplace_back(types::LetterInfo{
            .characterCode = code,
            .glyphOffsetPixels = glyphPos,
            .glyphSizePixels = glyph.metaInPixels.size,
            .bearingYPixels = glyph.metaInPixels.bearingY
        });

        glyphPos.x += glyph.metaInPixels.advance;

        // Count text size
        textSize.x = glm::max(textSize.x, currentLineWidth += glyph.metaInPixels.advance);
    });

    return { textInfo, textSize };
}

auto trc::ui::layoutText(const std::vector<CharCode>& chars, ::trc::ui32 fontIndex)
    -> std::pair<types::Text, ::trc::ivec2>
{
    types::Text textInfo{
        .fontIndex = fontIndex,
        .letters = {}
    };

    // Create letter descriptions
    const auto& face = FontRegistry::getFontInfo(fontIndex);
    const i32 lineHeight{ static_cast<i32>(face.maxAscend) - face.maxDescend };
    ivec2 glyphPos{ 0, face.maxAscend };
    ivec2 textSize{ 0, lineHeight };
    i32 currentLineWidth{ 0 };

    for (CharCode code : chars)
    {
        if (code == '\n')
        {
            glyphPos.x = 0.0f;
            glyphPos.y += face.lineSpace;

            // Count full text size
            textSize.y += lineHeight;
            currentLineWidth = 0;

            continue;
        }

        auto& glyph = FontRegistry::getGlyph(fontIndex, code);
        textInfo.letters.emplace_back(types::LetterInfo{
            .characterCode = code,
            .glyphOffsetPixels = glyphPos,
            .glyphSizePixels = glyph.metaInPixels.size,
            .bearingYPixels = glyph.metaInPixels.bearingY
        });

        glyphPos.x += glyph.metaInPixels.advance;

        // Count text size
        textSize.x = glm::max(textSize.x, currentLineWidth += glyph.metaInPixels.advance);
    }

    return { textInfo, textSize };
}



void trc::ui::StaticTextProperties::setDefaultFont(ui32 fontIndex)
{
    defaultFont = fontIndex;
}

auto trc::ui::StaticTextProperties::getDefaultFont() -> ui32
{
    return defaultFont;
}



trc::ui::Text::Text(std::string str, ui32 fontIndex)
    :
    printedText(std::move(str)),
    fontIndex(fontIndex)
{
    setSize({ 1.0f, 1.0f });
    setSizeProperties({ .format=Format::eNorm, .alignment=Align::eRelative });
}

void trc::ui::Text::draw(DrawList& drawList)
{
    drawList.emplace_back(DrawInfo{
        .pos   = globalPos,
        .size  = globalSize,
        .style = { .background = vec4(0.0f) },
        .type  = layoutText(printedText, fontIndex).first
    });
}

void trc::ui::Text::print(std::string str)
{
    printedText = std::move(str);
}
