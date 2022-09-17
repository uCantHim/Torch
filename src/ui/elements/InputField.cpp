#include "trc/ui/elements/InputField.h"

#include "trc/text/UnicodeUtils.h"
#include "trc/ui/Window.h"



trc::ui::InputField::InputField(Window& window)
    :
    Quad(window),
    text(window.create<Text>()),
    cursor(window.create<Line>())
{
    attach(text);
    text.attach(cursor);
    cursor.setSize(0.0f, 1.0f);
    cursor.setSizing(Format::eNorm, { Scale::eAbsolute, Scale::eRelativeToParent });
    cursor.style.background=vec4(1.0f);

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
            auto& io = this->window->getIoConfig();
            if (e.key == io.keyMap.backspace) {
                removeCharacterLeft();
            }
            else if (e.key == io.keyMap.del) {
                removeCharacterRight();
            }
            else if (e.key == io.keyMap.arrowLeft) {
                decCursorPos();
            }
            else if (e.key == io.keyMap.arrowRight) {
                incCursorPos();
            }
            else if (e.key == io.keyMap.enter) {
                //inputString += '\n';
            }
        }
    });
}

trc::ui::InputField::InputField(Window& window, ui32 fontIndex, ui32 fontSize)
    :
    InputField(window)
{
    this->fontIndex = fontIndex;
    this->fontSize = fontSize;
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
    textOffset = 0.0f;
}

void trc::ui::InputField::incCursorPos()
{
    if (cursorPosition < inputChars.size())
    {
        ++cursorPosition;
        positionText();
    }
}

void trc::ui::InputField::decCursorPos()
{
    if (cursorPosition > 0)
    {
        --cursorPosition;
        positionText();
    }
}

void trc::ui::InputField::positionText()
{
    if (!inputChars.empty())
    {
        const vec2 fontScaling = window->pixelsToNorm(vec2(fontSize));
        auto [text, _] = layoutText(inputChars, fontIndex, fontScaling);
        cursorPos = textOffset + text.letters.at(cursorPosition).glyphOffset.x;
    }
    else {
        cursorPos = 0.0f;
    }

    const float displayBegin = 0.0f;
    const float displayEnd = globalSize.x;
    if (cursorPos > displayEnd) {
        // Cursor is out-of-bounds to the right
        textOffset -= globalSize.x * 0.4f;
    }
    else if (cursorPos < displayBegin) {
        // Cursor is out-of-bounds to the left
        textOffset += globalSize.x * 0.4f;
    }

    text.setPos({ textOffset, Format::eNorm }, 0.0f);
    cursor.setPos(0.0f, 0.0f);
}

void trc::ui::InputField::inputCharacter(CharCode code)
{
    inputChars.insert(inputChars.begin() + cursorPosition, code);
    incCursorPos();
    text.print({ inputChars.begin(), inputChars.end() });

    event::Input event(inputChars, code);
    notify(event);
}

void trc::ui::InputField::removeCharacterLeft()
{
    if (cursorPosition != 0)
    {
        decCursorPos();
        inputChars.erase(inputChars.begin() + cursorPosition);
        text.print({ inputChars.begin(), inputChars.end() });

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
        text.print({ inputChars.begin(), inputChars.end() });

        if (eventOnDelete)
        {
            event::Input event(inputChars, window->getIoConfig().keyMap.backspace);
            notify(event);
        }
    }
}
