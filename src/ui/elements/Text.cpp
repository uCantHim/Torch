#include "ui/elements/Text.h"

#include "ui/DrawInfo.h"
#include "text/UnicodeUtils.h"



auto trc::ui::calcTextDrawable(const std::string& str, ::trc::ui32 fontIndex) -> types::Text
{
    types::Text textInfo{
        .fontIndex = fontIndex,
        .letters = {}
    };

    // Create letter descriptions
    const ui32 lineSpace = FontRegistry::getFontInfo(fontIndex).maxGlyphHeight;
    ivec2 glyphPos{ 0, 0 };

    iterUtf8(str, [&](CharCode code) {
        if (code == '\n')
        {
            glyphPos.x = 0.0f;
            glyphPos.y += lineSpace;
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
    });

    return textInfo;
}



trc::ui::Text::Text(std::string str, ui32 fontIndex)
    :
    printedText(std::move(str)),
    fontIndex(fontIndex)
{
    setSize({ 1.0f, 1.0f });
    setSizeProperties({ .type=SizeType::eNorm, .alignment=Align::eRelative });
}

void trc::ui::Text::draw(DrawList& drawList, vec2 globalPos, vec2 globalSize)
{
    drawList.emplace_back(DrawInfo{
        .pos=globalPos,
        .size=globalSize,
        .style = {
            .background = vec4(0.0f)
        },
        .type = calcTextDrawable(printedText, fontIndex)
    });
}

void trc::ui::Text::print(std::string str)
{
    printedText = std::move(str);
}
