#pragma once

#include <string>

#include "ui/Element.h"
#include "ui/FontRegistry.h"

namespace trc::ui
{
    class StaticTextProperties
    {
    public:
        static void setDefaultFont(ui32 fontIndex);
        static auto getDefaultFont() -> ui32;

    private:
        static inline ui32 defaultFont{ 0 };
    };

    auto calcTextDrawable(const std::string& str, ui32 fontIndex) -> types::Text;

    class Text : public Element, public StaticTextProperties
    {
    public:
        Text(std::string str, ui32 fontIndex = getDefaultFont());

        void draw(DrawList& drawList, vec2 globalPos, vec2 globalSize) override;

        void print(std::string str);

    private:
        std::string printedText;
        ui32 fontIndex;
    };
} // namespace trc::ui
