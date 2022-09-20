#include "trc/ui/elements/InputField.h"

#include "trc/text/UnicodeUtils.h"
#include "trc/ui/Window.h"



trc::ui::InputField::InputField(Window& window)
    :
    Quad(window),
    text(window.create<Text>()),
    cursor(window.create<Line>())
{
    this->setSize(getSize().x, getFontHeightLatinPixels(fontIndex) + 10);
    this->style.padding.set(kPaddingPixels);
    this->style.restrictDrawArea = true;

    attach(text);
    text.setPositionScaling(Scale::eAbsolute);
    cursor.setPositionScaling(Scale::eAbsolute);
    cursor.setSize(0.0f, getFontHeightLatin(fontIndex));
    cursor.setSizing({ Format::eNorm, Format::eNorm }, Scale::eAbsolute);
    cursor.style.background=vec4(1.0f);

    positionText();

    addEventListener([this](event::Click&) {
        attach(cursor);
        focused = true;
    });
    window.getRoot().addEventListener([this](event::Click&) {
        detach(cursor);
        focused = false;
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
    textOffset = vec2(0.0f);
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
    textOffset.y = (globalSize.y - globalSize.y * getFontHeightLatin(fontIndex)) * 0.5f;  // Center text

    const vec2 fontScaling = window->pixelsToNorm(vec2(fontSize));
    auto [layout, _] = layoutText(inputChars, fontIndex, fontScaling);
    const auto& currentLetter = layout.letters.at(cursorPosition);
    float cursorPos = textOffset.x + currentLetter.glyphOffset.x;

    const float padding = window->pixelsToNorm(kPaddingPixels).x;
    const float displayBegin = padding;
    const float displayEnd = globalSize.x - padding;
    if (cursorPosition > 0)
    {
        const auto& prevLetter = layout.letters.at(cursorPosition - 1);
        while (cursorPos > displayEnd)  // Cursor is out-of-bounds to the right
        {
            // Need the advance here, not the glyph size!
            textOffset.x -= currentLetter.glyphOffset.x - prevLetter.glyphOffset.x;
            cursorPos = textOffset.x + currentLetter.glyphOffset.x;
        }
        while (cursorPos < displayBegin)  // Cursor is out-of-bounds to the left
        {
            // Need the advance here, not the glyph size!
            textOffset.x += currentLetter.glyphOffset.x - prevLetter.glyphOffset.x;
            cursorPos = textOffset.x + currentLetter.glyphOffset.x;
        }
    }

    text.setPos(textOffset);
    cursor.setPos(cursorPos, textOffset.y);
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
