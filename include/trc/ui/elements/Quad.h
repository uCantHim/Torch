#pragma once

#include "../Element.h"

namespace trc::ui
{
    class Quad : public Element
    {
        void draw(DrawList& drawList, vec2 globalPos, vec2 globalSize) override
        {
            drawList.emplace_back(globalPos, globalSize, style, types::Quad{});
        }
    };
} // namespace trc::ui
