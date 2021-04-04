#pragma once

#include <string>
#include <atomic>

#include "Types.h"
#include "data_utils/IndexMap.h"
#include "text/GlyphLoading.h"

namespace trc::ui
{
    class FontRegistry
    {
    public:
        static auto addFont(const fs::path& file, ui32 fontSize) -> ui32;

        static auto getFontInfo(ui32 fontIndex) -> const Face&;
        static auto getGlyph(ui32 fontIndex, wchar_t character) -> const GlyphMeta&;

    private:
        struct FontData
        {
            Face face;
            ui32 index;

            data::IndexMap<wchar_t, u_ptr<GlyphMeta>> glyphs;
        };

        static inline std::atomic<ui32> nextFontIndex{ 0 };
        static inline data::IndexMap<ui32, u_ptr<FontData>> fonts;
    };
} // namespace trc::ui
