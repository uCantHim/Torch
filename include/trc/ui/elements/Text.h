#pragma once

#include <string>

#include "trc/ui/Element.h"
#include "trc/ui/elements/BaseElements.h"
#include "trc/ui/FontRegistry.h"

namespace trc::ui
{
    /**
     * @brief Generate glyph positions for a string of text
     *
     * Always adds a trailing null-character at the end. It has the correct
     * position that a normal character would have. It can be used to
     * compute things that require the advance of the last character in
     * the string.
     *
     * @param vec2 scaling The function uses the font's normalized glyph
     *        data. Set this scaling to your desired font size divided by
     *        your window size to get properly sized glyphs.
     */
    auto layoutText(const std::string& str, ui32 fontIndex, vec2 scaling)
        -> std::pair<types::Text, vec2>;

    /**
     * @brief Generate glyph positions for an array of unicode characters
     *
     * Always adds a trailing null-character at the end. It has the correct
     * position that a normal character would have. It can be used to
     * compute things that require the advance of the last character in
     * the string.
     *
     * @param vec2 scaling The function uses the font's normalized glyph
     *        data. Set this scaling to your desired font size divided by
     *        your window size to get properly sized glyphs.
     */
    auto layoutText(const std::vector<CharCode>& chars, ui32 fontIndex, vec2 scaling)
        -> std::pair<types::Text, vec2>;

    class Text : public Element, TextBase
    {
    public:
        Text() = default;
        Text(std::string str,
             ui32 fontIndex = DefaultStyle::font,
             ui32 fontSize = DefaultStyle::fontSize);

        void draw(DrawList& drawList) override;

        void print(std::string str);

    private:
        std::string printedText;
    };
} // namespace trc::ui
