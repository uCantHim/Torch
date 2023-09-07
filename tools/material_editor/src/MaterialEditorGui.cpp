#include "MaterialEditorGui.h"

#include <trc/ImguiIntegration.h>
#include <imgui.h>
namespace ig = ImGui;



MaterialEditorGui::MaterialEditorGui(
    const trc::Window& window,
    s_ptr<GraphManipulator> graphManip)
    :
    window(&window),
    graph(graphManip)
{
    const vec2 windowSize = window.getSize();
    menuBarSize = { windowSize.x * 0.2f, windowSize.y };
}

void MaterialEditorGui::drawGui()
{
    trc::imgui::beginImguiFrame();

    ig::SetNextWindowPos({ 0, 0 });
    ig::SetNextWindowSize({ menuBarSize.x, menuBarSize.y });
    const auto flags = ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoBringToFrontOnFocus
                     | ImGuiWindowFlags_NoTitleBar;
    if (ig::Begin("##matedit-menu", nullptr, flags))
    {
        // Apply resizes on the right window border
        if (ig::GetWindowPos().x == 0) {
            menuBarSize.x = ig::GetWindowWidth();
        }

        ig::Text("Hello menu! :D");
        ig::End();
    }
}
