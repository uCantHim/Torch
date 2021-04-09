#include "ui/elements/InputField.h"

#include "text/UnicodeUtils.h"
#include "ui/Window.h"



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

    const vec2 normPadding = paddingType == Format::eNorm
        ? padding
        : window->pixelsToNorm(padding);
    const vec2 fontScaling = window->pixelsToNorm(vec2(fontSize));
    auto [text, textSize] = layoutText(inputChars, fontIndex, fontScaling);
    const vec2 textPos{ globalPos + normPadding };

    globalSize.y = textSize.y + normPadding.y * 2.0f;
    Quad::draw(drawList);

    // Draw cursor line
    if (focused)
    {
        vec2 cursorPos = textPos;
        cursorPos.x += text.letters.at(cursorPosition).glyphOffset.x;

        drawList.push_back(DrawInfo{
            .pos   = cursorPos,
            .size  = { 0.0f, textSize.y },
            .style = ElementStyle{ .background=vec4(1.0f) },
            .type  = types::Line{ .width=1 }
        });
    }

    // Draw text
    text.maxDisplayWidth = globalSize.x - normPadding.x * 2.0f;
    drawList.push_back(DrawInfo{
        .pos   = textPos,
        .size  = textSize,
        .style = this->style,
        .type  = std::move(text)
    });
}

void trc::ui::InputField::setPadding(vec2 newPadding)
{
    padding = newPadding;
}

void trc::ui::InputField::setPaddingType(Format size)
{
    paddingType = size;
}

auto trc::ui::InputField::getText() const -> std::string
{
    return encodeUtf8(inputChars);
}

auto trc::ui::InputField::getChars() const -> const std::vector<CharCode>&
{
    return inputChars;
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
