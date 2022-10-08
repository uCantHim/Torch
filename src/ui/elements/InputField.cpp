#include "trc/ui/elements/InputField.h"

#include "trc/text/UnicodeUtils.h"
#include "trc/ui/Window.h"



trc::ui::InputField::InputField()
{
    addEventListener([this](event::Click&) {
        focused = true;
    });

    addEventListener([this](event::CharInput& e) {
        if (focused) {
            inputCharacter(decodeUtf8(e.character));
        }
    });

    addEventListener([this](event::KeyPress& e) {
        if (focused)
        {
            auto& io = window->getIoConfig();
            if (e.key == io.keyMap.backspace) {
                removeCharacterLeft();
            }
            else if (e.key == io.keyMap.del) {
                removeCharacterRight();
            }
            else if (e.key == io.keyMap.arrowLeft && cursorPosition > 0) {
                --cursorPosition;
            }
            else if (e.key == io.keyMap.arrowRight && cursorPosition < inputChars.size()) {
                ++cursorPosition;
            }
            else if (e.key == io.keyMap.enter) {
                //inputString += '\n';
            }
        }
    });
}

trc::ui::InputField::InputField(ui32 fontIndex, ui32 fontSize)
    :
    InputField()
{
    this->fontIndex = fontIndex;
    this->fontSize = fontSize;
}

void trc::ui::InputField::draw(DrawList& drawList)
{
    using namespace std::chrono;

    const vec2 fontScaling = window->pixelsToNorm(vec2(fontSize));
    auto [text, textSize] = layoutText(inputChars, fontIndex, fontScaling);
    const vec2 normPadding = getNormPadding(textSize, *window);

    globalSize.y = textSize.y + normPadding.y * 2.0f;
    Quad::draw(drawList);

    // Set scissor rect size
    text.displayBegin = globalPos.x + normPadding.x;
    text.maxDisplayWidth = globalSize.x - normPadding.x * 2.0f;

    // Cursor position is necessary for text offset calculation
    vec2 textPos{ globalPos + normPadding };
    vec2 cursorPos = textPos;
    cursorPos.x += text.letters.at(cursorPosition).glyphOffset.x;

    // Move the text if the cursor is out of bounds
    {
        float textOffset{ lastTextOffset };
        const float displayEnd = text.displayBegin + text.maxDisplayWidth;
        const float withLastOffset = cursorPos.x + lastTextOffset;
        if (withLastOffset > displayEnd) {
            // Cursor is out-of-bounds to the right
            textOffset -= withLastOffset - displayEnd;
        }
        else if (withLastOffset < text.displayBegin) {
            // Cursor is out-of-bounds to the left
            textOffset += text.displayBegin - withLastOffset;
        }

        lastTextOffset = textOffset;
        cursorPos.x += textOffset;
        textPos.x += textOffset;
    }

    // Draw cursor line
    if (focused)
    {
        drawList.push_back(DrawInfo{
            .pos   = cursorPos,
            .size  = { 0.0f, textSize.y },
            .style = ElementStyle{ .background=vec4(1.0f) },
            .type  = types::Line{ .width=1 }
        });
    }

    // Draw text
    drawList.push_back(DrawInfo{
        .pos   = textPos,
        .size  = textSize,
        .style = this->style,
        .type  = std::move(text)
    });
}

auto trc::ui::InputField::getInputText() const -> std::string
{
    return encodeUtf8(inputChars);
}

auto trc::ui::InputField::getInputChars() const -> const std::vector<CharCode>&
{
    return inputChars;
}

void trc::ui::InputField::clearInput()
{
    inputChars.clear();
    cursorPosition = 0;
    lastTextOffset = 0.0f;
}

void trc::ui::InputField::inputCharacter(CharCode code)
{
    inputChars.insert(inputChars.begin() + cursorPosition, code);
    cursorPosition++;

    event::Input event(inputChars, code);
    notify(event);
}

void trc::ui::InputField::removeCharacterLeft()
{
    if (cursorPosition != 0)
    {
        inputChars.erase(inputChars.begin() + (--cursorPosition));

        if (eventOnDelete)
        {
            event::Input event(inputChars, window->getIoConfig().keyMap.backspace);
            notify(event);
        }
    }
}

void trc::ui::InputField::removeCharacterRight()
{
    if (cursorPosition != inputChars.size())
    {
        // Don't decrement cursor position after delete
        inputChars.erase(inputChars.begin() + cursorPosition);

        if (eventOnDelete)
        {
            event::Input event(inputChars, window->getIoConfig().keyMap.backspace);
            notify(event);
        }
    }
}
