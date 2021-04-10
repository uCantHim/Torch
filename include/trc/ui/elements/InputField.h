#pragma once

#include <string>
#include <vector>

#include "ui/Element.h"
#include "ui/elements/Quad.h"
#include "ui/elements/Text.h"
#include "ui/elements/BaseElements.h"
#include "ui/event/InputEvent.h"

namespace trc::ui
{
    class InputField : public Quad, public TextBase, public Paddable
    {
    public:
        InputField();
        InputField(ui32 fontIndex, ui32 fontSize);

        void draw(DrawList& drawList) override;

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
        void inputCharacter(CharCode code);
        void removeCharacterLeft();
        void removeCharacterRight();

        bool focused{ false };
        std::vector<CharCode> inputChars;
        ui32 cursorPosition{ 0 };
        float lastTextOffset{ 0.0f };

        bool eventOnDelete{ true };
    };
} // namespace trc::ui
