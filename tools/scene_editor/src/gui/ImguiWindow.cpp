#include "gui/ImguiWindow.h"

#include <imgui.h>
namespace ig = ImGui;



ImguiWindow::ImguiWindow(std::string windowName, ImguiWindowType type)
    : name(std::move(windowName))
{
    setWindowType(type);
}

void ImguiWindow::setWindowType(ImguiWindowType type)
{
    switch (type)
    {
    case ImguiWindowType::eViewport:
        windowFlags = ImGuiWindowFlags_NoResize
                    | ImGuiWindowFlags_NoMove
                    | ImGuiWindowFlags_NoCollapse
                    | ImGuiWindowFlags_NoDecoration
                    | ImGuiWindowFlags_NoNavFocus
                    | ImGuiWindowFlags_NoBringToFrontOnFocus;
        break;
    case ImguiWindowType::eFloating:
        windowFlags = ImGuiWindowFlags_None;
        break;
    }

    windowType = type;
}

void ImguiWindow::draw(trc::Frame&)
{
    auto resizeFlags = ImGuiCond_Always;
    if (windowType == ImguiWindowType::eFloating)
    {
        resizeFlags = ImGuiCond_FirstUseEver;
        ig::SetNextWindowFocus();
    }

    const vec2 pos = viewportArea.pos;
    const vec2 size = viewportArea.size;
    ig::SetNextWindowPos({ pos.x, pos.y }, resizeFlags);
    ig::SetNextWindowSize({ size.x, size.y }, resizeFlags);
    ig::Begin(name.c_str(), nullptr, windowFlags);

    this->drawWindowContent();

    if (!ig::IsWindowCollapsed())
    {
        auto pos = ig::GetWindowPos();
        auto size = ig::GetWindowSize();
        viewportArea = { { pos.x, pos.y }, { size.x, size.y } };
    }

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
