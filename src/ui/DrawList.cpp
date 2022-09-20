#include "trc/ui/DrawList.h"

#include "trc/ui/Element.h"
#include "trc/ui/Window.h"



namespace trc::ui
{

DrawList::DrawList(const Window& window)
    :
    window(window)
{
    drawGroups.emplace_back(ScissorRect{ { 0, 0 }, window.getSize() });
}

bool DrawList::empty() const
{
    return drawGroups.empty();
}

void DrawList::push(types::Line prim, Element& el)
{
    drawGroups.back().lines.push_back({ el.globalPos, el.globalSize, el.style, std::move(prim) });
}

void DrawList::push(types::Quad prim, Element& el)
{
    drawGroups.back().quads.push_back({ el.globalPos, el.globalSize, el.style, std::move(prim) });
}

void DrawList::push(types::Text prim, Element& el)
{
    drawGroups.back().texts.push_back({ el.globalPos, el.globalSize, el.style, std::move(prim) });
}

void DrawList::pushScissorRect(Vec2D<pix_or_norm> origin, Vec2D<pix_or_norm> size)
{
    auto [ox, oy] = origin;
    auto [sx, sy] = size;

    drawGroups.emplace_back(ScissorRect{
        { ox.format == Format::ePixel ? ox.value : (ox.value * window.getSize().x),
          oy.format == Format::ePixel ? oy.value : (oy.value * window.getSize().y) },
        { sx.format == Format::ePixel ? sx.value : (sx.value * window.getSize().x),
          sy.format == Format::ePixel ? sy.value : (sy.value * window.getSize().y) },
    });
}

void DrawList::popScissorRect()
{
    drawGroups.emplace_back(ScissorRect{ { 0, 0 }, window.getSize() });
}

void DrawList::clear()
{
    drawGroups.clear();
    drawGroups.emplace_back(ScissorRect{ { 0, 0 }, window.getSize() });
}

auto DrawList::begin() -> iterator
{
    return drawGroups.begin();
}

auto DrawList::end() -> iterator
{
    return drawGroups.end();
}

auto DrawList::begin() const -> const_iterator
{
    return drawGroups.begin();
}

auto DrawList::end() const -> const_iterator
{
    return drawGroups.end();
}

} // namespace trc::ui
