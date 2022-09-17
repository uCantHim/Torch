#include "trc/ui/elements/Line.h"



namespace trc::ui
{

Line::Line(Window& window)
    : Element(window)
{
    style.padding.set(0.0f, 0.0f);
}

void Line::draw(DrawList& drawList)
{
    drawList.push(types::Line{ .width=lineWidth }, *this);
}

void Line::setWidth(ui32 newWidth)
{
    lineWidth = newWidth;
}

} // namespace trc::ui
