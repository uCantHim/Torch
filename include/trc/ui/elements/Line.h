#pragma once

#include "trc/ui/Element.h"

namespace trc::ui
{
    class Line : public Element
    {
    public:
        explicit Line(Window& window);

        void draw(DrawList& drawList) override;

        void setWidth(ui32 newWidth);

    private:
        ui32 lineWidth{ 1 };
    };
} // namespace trc::ui
