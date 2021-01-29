#pragma once

#include <string>
#include <locale>
#include <codecvt>

#include "ui/Element.h"
#include "ui/Font.h"

namespace trc::ui
{
    class StaticTextProperties
    {
    public:
        static void setDefaultFont(ui32 fontIndex);
        static auto getDefaultFont() -> ui32;

    protected:
        static inline std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstringConverter;

    private:
        static inline ui32 defaultFont{ 0 };
    };

    class Text : public Element, public StaticTextProperties
    {
    public:
        Text(std::string str, ui32 fontIndex = getDefaultFont());
        Text(std::wstring str, ui32 fontIndex = getDefaultFont());

        void draw(DrawList& drawList, vec2 globalPos, vec2 globalSize) override;

        void print(std::string str);
        void print(std::wstring str);

    private:
        std::wstring printedText;
        ui32 fontIndex;
    };
} // namespace trc::ui
