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

    struct NoType {};

    struct LetterInfo
    {
        vec2 texCoordLL;
        vec2 texCoordUR;
        vec2 glyphPosOffset;
        vec2 glyphSize;
        float bearingY;
    };

    struct TextDrawInfo
    {
        ui32 fontIndex;
        std::vector<LetterInfo> letters;
    };

    using TypedDrawInfo = std::variant<
        NoType,
        TextDrawInfo
    >;


    // --- Generic draw information ---

    struct DrawInfo
    {
        vec2 pos;
        vec2 size;

        // ui32 borderThickness{ 0 };

        std::variant<vec4, TextureInfo> color;
        TypedDrawInfo typeInfo;
    };

    class Drawable
    {
    private:
        friend class Window;

        virtual void draw(std::vector<DrawInfo>& drawList, vec2 globalPos, vec2 globalSize) = 0;
    };
} // namespace trc::ui
