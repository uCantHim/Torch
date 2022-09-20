#pragma once

#include <string>
#include <vector>

#include "trc/ui/Element.h"
#include "trc/ui/Window.h"
#include "trc/ui/elements/Line.h"
#include "trc/ui/elements/Quad.h"
#include "trc/ui/elements/Text.h"
#include "trc/ui/event/InputEvent.h"

namespace trc::ui
{
    class InputField : public Quad
    {
    public:
        explicit InputField(Window& window);
        InputField(Window& window, ui32 fontIndex, ui32 fontSize);

        /**
         * @return std::string UTF-8 encoded string
         */
        auto getInputText() const -> std::string;

        /**
         * @return const std::vector<CharCode>& Unicode character codes
         */
        auto getInputChars() const -> const std::vector<CharCode>&;

        void clearInput();

        /**
         * Also dispatch event::Input events when a character is deleted.
         * This is the default setting.
         */
        void enableEventOnDelete();

        /**
         * Don't dispatch event::Input events when a character is deleted.
         */
        void disableEventOnDelete();

    private:
        static constexpr vec2 kPaddingPixels{ 5, 5 };

        void incCursorPos();
        void decCursorPos();
        void positionText();
        void inputCharacter(CharCode code);
        void removeCharacterLeft();
        void removeCharacterRight();

        bool focused{ false };
        std::vector<CharCode> inputChars;
        ui32 cursorPosition{ 0 };
        vec2 textOffset{ 0.0f };

        bool eventOnDelete{ true };

        UniqueElement<Text> text;
        UniqueElement<Line> cursor;
    };
} // namespace trc::ui
