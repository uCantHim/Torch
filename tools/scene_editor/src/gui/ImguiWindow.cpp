#include "gui/ImguiWindow.h"

#include <imgui.h>
namespace ig = ImGui;



ImguiWindow::ImguiWindow(std::string windowName, ImguiWindowType type)
    : name(std::move(windowName))
{
    setWindowType(type);
}

auto ImguiWindow::notify(const UserInput&) -> NotifyResult
{
    return NotifyResult::eRejected;
}

auto ImguiWindow::notify(const Scroll&) -> NotifyResult
{
    return NotifyResult::eRejected;
}

auto ImguiWindow::notify(const CursorMovement&) -> NotifyResult
{
    return NotifyResult::eRejected;
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
    if (wasResizedExternally)
    {
        const vec2 pos = viewportArea.pos;
        const vec2 size = viewportArea.size;
        ig::SetNextWindowPos({ pos.x, pos.y });
        if (windowType != ImguiWindowType::eFloating) {
            ig::SetNextWindowSize({ size.x, size.y });
        }
        wasResizedExternally = false;
    }

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
    wasResizedExternally = true;
}

auto ImguiWindow::getSize() -> ViewportArea
{
    return viewportArea;
}
