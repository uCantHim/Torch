#include "trc/ui/elements/Text.h"

#include <iostream>

#include "trc/text/UnicodeUtils.h"
#include "trc/ui/DrawInfo.h"
#include "trc/ui/Window.h"



namespace trc::ui
{

auto layoutText(const std::string& str, ui32 fontIndex, vec2 scaling)
    -> std::pair<types::Text, vec2>
{
    return layoutText(decodeUtf8(str), fontIndex, scaling);
}

auto layoutText(const std::vector<CharCode>& chars, ui32 fontIndex, vec2 scaling)
    -> std::pair<types::Text, vec2>
{
    types::Text textInfo{
        .fontIndex = fontIndex,
        .letters = {}
    };

    // Create letter descriptions
    const auto& face = FontRegistry::getFontInfo(fontIndex);

    vec2 glyphPos{ 0, face.maxAscendNorm };
    vec2 textSize{ 0, face.maxAscendNorm + face.maxDescendNorm };
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

auto getFontHeight(ui32 fontIndex) -> float
{
    const auto& face = FontRegistry::getFontInfo(fontIndex);
    return face.maxAscendNorm + face.maxDescendNorm;
}

auto getFontHeightPixels(ui32 fontIndex) -> float
{
    const auto& face = FontRegistry::getFontInfo(fontIndex);
    return face.maxAscend + face.maxDescend;
}



// ---------------------- //
//      Text element      //
// ---------------------- //

Text::Text(Window& window)
    :
    Text(window, "")
{
}

Text::Text(Window& window, std::string str, ui32 fontIndex, ui32 fontSize)
    :
    Element(window),
    TextBase(fontIndex, fontSize),
    textElem(window.create<Letters>())
{
    style.dynamicSize = true;

    textElem.setSize(0.0f, getFontHeight(fontIndex));
    textElem.setSizing(Format::eNorm, Scale::eAbsolute);
    attach(textElem);

    print(std::move(str));
}

void Text::print(std::string str)
{
    printedText = std::move(str);

    vec2 scaling = window->pixelsToNorm(vec2(fontSize));
    auto [text, size] = layoutText(printedText, fontIndex, scaling);

    textElem.setSize(size);
    textElem.setText(std::move(text));
}

Text::Letters::Letters(Window& window)
    :
    Element(window)
{
}

void Text::Letters::draw(DrawList& drawList)
{
    drawList.push(text, *this);
}

void Text::Letters::setText(types::Text newText)
{
    text = std::move(newText);
}

} // namespace trc::ui
