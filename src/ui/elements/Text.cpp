#include "ui/elements/Text.h"

#include <iostream>

#include "text/UnicodeUtils.h"
#include "ui/DrawInfo.h"
#include "ui/Window.h"



auto trc::ui::layoutText(const std::string& str, ::trc::ui32 fontIndex, ::trc::vec2 scaling)
    -> std::pair<types::Text, ::trc::vec2>
{
    return layoutText(decodeUtf8(str), fontIndex, scaling);
}

auto trc::ui::layoutText(const std::vector<CharCode>& chars, ::trc::ui32 fontIndex, ::trc::vec2 scaling)
    -> std::pair<types::Text, ::trc::vec2>
{
    types::Text textInfo{
        .fontIndex = fontIndex,
        .letters = {}
    };

    // Create letter descriptions
    const auto& face = FontRegistry::getFontInfo(fontIndex);

    vec2 glyphPos{ 0, face.maxAscendNorm };
    vec2 textSize{ 0, face.maxLineHeightNorm };
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

        auto& glyph = FontRegistry::getGlyph(fontIndex, code);
        textInfo.letters.emplace_back(types::LetterInfo{
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
    textInfo.letters.emplace_back(types::LetterInfo{
        .characterCode = 0x00,
        .glyphOffset = glyphPos * scaling,
        .glyphSize = vec2(0.0f),
        .bearingY = 0.0f
    });

    return { textInfo, textSize * scaling };
}



void trc::ui::StaticTextProperties::setDefaultFont(ui32 fontIndex)
{
    defaultFont = fontIndex;
}

auto trc::ui::StaticTextProperties::getDefaultFont() -> ui32
{
    return defaultFont;
}



// ------------------- //
//      Text base      //
// ------------------- //

trc::ui::TextBase::TextBase(std::string str)
    :
    printedText(std::move(str))
{
}

trc::ui::TextBase::TextBase(std::string str, ui32 fontIndex, ui32 fontSize)
    :
    printedText(std::move(str)),
    fontIndex(fontIndex),
    fontSize(fontSize)
{
}

void trc::ui::TextBase::print(std::string str)
{
    printedText = std::move(str);
}

auto trc::ui::TextBase::getFontScaling(const Window& window) const -> vec2
{
    return window.pixelsToNorm(vec2(fontSize));
}



// ---------------------- //
//      Text element      //
// ---------------------- //

trc::ui::Text::Text(std::string str, ui32 fontIndex)
    :
    Text(std::move(str), fontIndex, FontRegistry::getFontInfo(fontIndex).renderSize)
{
}

trc::ui::Text::Text(std::string str, ui32 fontIndex, ui32 fontSize)
    :
    TextBase(std::move(str), fontIndex, fontSize)
{
    setSize({ 1.0f, 1.0f });
    setSizeProperties({ .format=Format::eNorm, .alignment=Align::eRelative });
}

void trc::ui::Text::draw(DrawList& drawList)
{
    vec2 scaling = window->pixelsToNorm(vec2(FontRegistry::getFontInfo(fontIndex).renderSize * 1.333f));
    drawList.emplace_back(DrawInfo{
        .pos   = globalPos,
        .size  = globalSize,
        .style = { .background = vec4(0.0f) },
        .type  = layoutText(printedText, fontIndex, scaling).first
    });
}
