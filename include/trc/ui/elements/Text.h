#pragma once

#include <string>

#include "trc/ui/DrawInfo.h"
#include "trc/ui/Element.h"
#include "trc/ui/Window.h"

namespace trc::ui
{
    class FontLayouter;

    class Text : public Element
    {
    public:
        explicit Text(Window& window);
        Text(Window& window,
             std::string str,
             ui32 fontIndex = DefaultStyle::font,
             ui32 fontSize = DefaultStyle::fontSize);

        void draw(DrawList&) override {}

        void print(std::string str);

    private:
        class Letters : public Element
        {
        public:
            explicit Letters(Window& window);

            void draw(DrawList& drawList) override;

            void setText(types::Text newText);

        private:
            types::Text text;
        };

        auto getFontLayouter() -> FontLayouter&;

        std::string printedText;
        UniqueElement<Letters> textElem;
    };
} // namespace trc::ui
