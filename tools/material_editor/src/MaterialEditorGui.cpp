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
        // Apply resizes only on the right window border
        if (ig::GetWindowPos().x == 0) {
            menuBarSize.x = ig::GetWindowWidth();
        }

        ig::Text("Hello menu! :D");
        ig::End();
    }

    // Context menu
    const auto popupFlags = ImGuiWindowFlags_NoMove
                          | ImGuiWindowFlags_NoResize
                          | ImGuiWindowFlags_NoDecoration
                          | ImGuiWindowFlags_NoTitleBar;
    if (contextMenuIsOpen)
    {
        ig::SetNextWindowPos({ contextMenuPos.x, contextMenuPos.y });
        ig::PushStyleVar(ImGuiStyleVar_Alpha, kContextMenuAlpha);
        if (ig::Begin("##matedit-context-menu", nullptr, popupFlags))
        {
            ig::Text("Hello popup!");
            ig::End();
        }
        ig::PopStyleVar();
    }
}

void MaterialEditorGui::openContextMenu(vec2 position)
{
    contextMenuPos = position;
    contextMenuIsOpen = true;
}

void MaterialEditorGui::closeContextMenu()
{
    contextMenuIsOpen = false;
}
