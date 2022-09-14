#include "ui/DrawList.h"



namespace trc::ui
{

bool DrawList::empty() const
{
    return items.empty();
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
