#pragma once

#include <string>

#include "ui/Element.h"
#include "ui/FontRegistry.h"

namespace trc::ui
{
    /**
     * Generate glyph positions for a string of text
     *
     * @param vec2 scaling The function uses the font's normalized glyph
     *        data. Set this scaling to your desired font size divided by
     *        your window size to get properly sized glyphs.
     */
    auto layoutText(const std::string& str, ui32 fontIndex, vec2 scaling)
        -> std::pair<types::Text, vec2>;

    /**
     * Generate glyph positions for an array of unicode characters
     *
     * @param vec2 scaling The function uses the font's normalized glyph
     *        data. Set this scaling to your desired font size divided by
     *        your window size to get properly sized glyphs.
     */
    auto layoutText(const std::vector<CharCode>& chars, ui32 fontIndex, vec2 scaling)
        -> std::pair<types::Text, vec2>;

    class StaticTextProperties
    {
    public:
        static void setDefaultFont(ui32 fontIndex);
        static auto getDefaultFont() -> ui32;

    private:
        static inline ui32 defaultFont{ 0 };
    };

    class TextBase : public StaticTextProperties
    {
    public:
        TextBase() = default;
        explicit TextBase(std::string str);
        TextBase(std::string str, ui32 fontIndex, ui32 fontSize);

        void print(std::string str);

    protected:
        auto getFontScaling(const Window& window) const -> vec2;

        std::string printedText;
        ui32 fontIndex{ getDefaultFont() };
        ui32 fontSize{ FontRegistry::getFontInfo(fontIndex).renderSize };
    };

    class Text : public Element, public TextBase
    {
    public:
        Text() = default;
        Text(std::string str, ui32 fontIndex);
        Text(std::string str, ui32 fontIndex, ui32 fontSize);

        void draw(DrawList& drawList) override;
    };
} // namespace trc::ui
