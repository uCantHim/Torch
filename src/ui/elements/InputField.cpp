#include "trc/ui/elements/InputField.h"

#include "trc/text/UnicodeUtils.h"



trc::ui::InputField::InputField(Window& window)
    :
    Quad(window),
    text(window.makeUnique<Text>()),
    cursor(window.makeUnique<Line>())
{
    this->setSize(getSize().x, window.getFontLayouter().getFontHeightLatinPixels(style.fontIndex) + 10);
    this->style.padding.set(kPaddingPixels);
    this->style.restrictDrawArea = true;

    attach(*text);
    text->setPositionScaling(Scale::eAbsolute);
    cursor->style.background=vec4(1.0f);
    cursor->setPositioning(Format::eNorm, Scale::eAbsolute);
    cursor->setSize(0.0f, 1.0f);

    positionText();

    addEventListener([this](event::Click&) {
        attach(*cursor);
        focused = true;
    });
    window.getRoot().addEventListener([this](event::Click&) {
        detach(*cursor);
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
    style.fontIndex = fontIndex;
    style.fontSize = fontSize;
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
    // Center text
    textOffset.y = (globalSize.y - globalSize.y * window->getFontLayouter().getFontHeightLatin(style.fontIndex)) * 0.5f;

    const vec2 fontScaling = window->pixelsToNorm(vec2(style.fontSize));
    const auto [layout, _] = window->getFontLayouter().layoutText(inputChars, style.fontIndex, fontScaling);

    const auto& currentLetter = layout.letters.at(cursorPosition);
    float cursorPos = textOffset.x + currentLetter.glyphOffset.x;

    const float padding = style.padding.calcNormalizedPadding(*window).x;
    const float displayBegin = 0;
    const float displayEnd = globalSize.x - padding * 2.0f;
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

    text->setPos(textOffset);
    cursor->setPos(cursorPos, 0.0f);
}

void trc::ui::InputField::inputCharacter(CharCode code)
{
    inputChars.insert(inputChars.begin() + cursorPosition, code);
    incCursorPos();
    text->print({ inputChars.begin(), inputChars.end() });

    event::Input event(inputChars, code);
    notify(event);
}

void trc::ui::InputField::removeCharacterLeft()
{
    if (cursorPosition != 0)
    {
        decCursorPos();
        inputChars.erase(inputChars.begin() + cursorPosition);
        text->print({ inputChars.begin(), inputChars.end() });

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
        text->print({ inputChars.begin(), inputChars.end() });

        if (eventOnDelete)
        {
            event::Input event(inputChars, window->getIoConfig().keyMap.backspace);
            notify(event);
        }
    }
}
