#pragma once

#include <variant>

#include "Types.h"
#include "Transform.h"

namespace trc::ui
{
    // --- Reusable data structures ---

    struct TextureInfo
    {
        vec2 uvLL;
        vec2 uvUR;
        ui32 textureIndex;
    };


    // --- Typed draw information ---
    namespace types
    {
        struct NoType {};

        struct LetterInfo
        {
            vec2 texCoordLL;
            vec2 texCoordUR;
            vec2 glyphPosOffset;
            vec2 glyphSize;
            float bearingY;
        };

        struct Text
        {
            ui32 fontIndex;
            std::vector<LetterInfo> letters;
        };
    }


    /**
     * Generic draw information for all elements
     */
    struct ElementDrawInfo
    {
        vec2 pos;
        vec2 size;

        // ui32 borderThickness{ 0 };

        std::variant<vec4, TextureInfo> background;
    };

    using DrawType = std::variant<
        types::NoType,
        types::Text
    >;

    /**
     * All draw information for a gui element
     */
    struct DrawInfo
    {
        ElementDrawInfo elem;
        DrawType type;
    };

    using DrawList = std::vector<DrawInfo>;

    class Drawable
    {
    protected:
        friend class Window;

        virtual void draw(DrawList& drawList, vec2 globalPos, vec2 globalSize) = 0;
    };
} // namespace trc::ui
