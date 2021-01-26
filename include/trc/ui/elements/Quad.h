#pragma once

#include "../Element.h"

namespace trc::ui
{
    class Quad : public Element
    {
        void draw(DrawList& drawList, vec2 globalPos, vec2 globalSize) override
        {
            drawList.emplace_back(
                ElementDrawInfo{
                    .pos=globalPos,
                    .size=globalSize,
                    .background=vec4(1.0f)
                },
                types::NoType{}
            );
        }
    };
} // namespace trc::ui
