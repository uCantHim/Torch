#include "ui/DrawList.h"

#include "ui/Element.h"



namespace trc::ui
{

bool DrawList::empty() const
{
    return items.empty();
}

void DrawList::push(types::Line prim, Element& el)
{
    push_back({ el.globalPos, el.globalSize, el.style, std::move(prim) });
}

void DrawList::push(types::Quad prim, Element& el)
{
    push_back({ el.globalPos, el.globalSize, el.style, std::move(prim) });
}

void DrawList::push(types::Text prim, Element& el)
{
    push_back({ el.globalPos, el.globalSize, el.style, std::move(prim) });
}

void DrawList::push_back(DrawInfo draw)
{
    items.emplace_back(std::move(draw));
}

void DrawList::clear()
{
    items.clear();
}

auto DrawList::begin() -> iterator
{
    return items.begin();
}

auto DrawList::end() -> iterator
{
    return items.end();
}

} // namespace trc::ui
