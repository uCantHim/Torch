#pragma once

#include <string>

#include "ui/Element.h"
#include "ui/FontRegistry.h"

namespace trc::ui
{
    auto layoutText(const std::string& str, ui32 fontIndex) -> std::pair<types::Text, ivec2>;
    auto layoutText(const std::vector<CharCode>& chars, ui32 fontIndex) -> std::pair<types::Text, ivec2>;

    class StaticTextProperties
    {
    public:
        static void setDefaultFont(ui32 fontIndex);
        static auto getDefaultFont() -> ui32;

    private:
        static inline ui32 defaultFont{ 0 };
    };

    class Text : public Element, public StaticTextProperties
    {
    public:
        Text(std::string str, ui32 fontIndex = getDefaultFont());

        void draw(DrawList& drawList) override;

        void print(std::string str);

    private:
        std::string printedText;
        ui32 fontIndex;
    };
} // namespace trc::ui
