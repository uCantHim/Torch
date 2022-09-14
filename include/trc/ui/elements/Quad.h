#pragma once

#include "trc/ui/Element.h"

namespace trc::ui
{
    class Quad : public Element
    {
    protected:
        void draw(DrawList& drawList) override
        {
            drawList.push_back(DrawInfo{ globalPos, globalSize, style, types::Quad{} });
        }
    };
} // namespace trc::ui
