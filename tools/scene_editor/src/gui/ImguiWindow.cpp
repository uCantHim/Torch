#include "gui/ImguiWindow.h"

#include <imgui.h>
namespace ig = ImGui;



ImguiWindow::ImguiWindow(std::string windowName)
    : name(std::move(windowName))
{}

void ImguiWindow::draw(trc::Frame&)
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
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoNavFocus
        | ImGuiWindowFlags_NoBringToFrontOnFocus
    );
    this->drawWindowContent();
    ig::End();
}

void ImguiWindow::resize(const ViewportArea& newArea)
{
    viewportArea = newArea;
}

auto ImguiWindow::getSize() -> ViewportArea
{
    return viewportArea;
}
