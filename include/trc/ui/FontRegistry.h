#pragma once

#include <string>
#include <atomic>

#include <nc/data/IndexMap.h>

#include "Types.h"
#include "text/GlyphLoading.h"

namespace trc::ui
{
    class FontRegistry
    {
    public:
        static auto addFont(const fs::path& file, ui32 fontSize) -> ui32;

        static auto getFontInfo(ui32 fontIndex) -> const Face&;
        static auto getGlyph(ui32 fontIndex, CharCode character) -> const GlyphMeta&;

        static void setFontAddCallback(std::function<void(ui32, const GlyphCache&)> func);

    private:
        static inline std::atomic<ui32> nextFontIndex{ 0 };
        static inline data::IndexMap<ui32, u_ptr<GlyphCache>> fonts;

        static inline std::function<void(ui32, const GlyphCache&)> onFontAdd{ [](auto&&, auto&&) {} };
    };
} // namespace trc::ui
