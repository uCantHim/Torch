#include "viewport/ImGuiViewport.h"

#include <imgui.h>
namespace ig = ImGui;



ImGuiWindow::ImGuiWindow(std::string windowName)
    : name(std::move(windowName))
{}

void ImGuiWindow::draw(trc::Frame&)
{
    const vec2 pos = viewportArea.pos;
    const vec2 size = viewportArea.size;

    ig::SetNextWindowPos({ pos.x, pos.y });
    ig::SetNextWindowSize({ size.x, size.y });
    ig::Begin(
        name.c_str(),
        nullptr,
        ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoNavFocus
    );
    this->drawWindowContent();
    ig::End();
}

void ImGuiWindow::resize(const ViewportArea& newArea)
{
    viewportArea = newArea;
}

auto ImGuiWindow::getSize() -> ViewportArea
{
    return viewportArea;
}
