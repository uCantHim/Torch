#include "MaterialEditorGui.h"

#include <trc/ImguiIntegration.h>
#include <imgui.h>
namespace ig = ImGui;

#include "ManipulationActions.h"
#include "MaterialNode.h"



MaterialEditorGui::MaterialEditorGui(
    const trc::Window& window,
    s_ptr<GraphManipulator> graphManip)
    :
    graph(graphManip)
{
    menuBarSize = { window.getSize().x * 0.2f, window.getSize().y };

    trc::on<trc::SwapchainResizeEvent>([&, this](auto& e) {
        if (e.swapchain == &window) {
            menuBarSize = { window.getSize().x * 0.2f, window.getSize().y };
        }
    });
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

        drawMainMenuContents();
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

void MaterialEditorGui::drawMainMenuContents()
{
    ig::Text("Hello menu! :D");
    ig::Separator();

    for (const auto& desc : getMaterialNodes())
    {
        if (ig::Button("Create")) {
            graph->applyAction(std::make_unique<action::CreateNode>(desc));
        }
        ig::SameLine();
        ig::Text("%s", desc.name.c_str());
    }
}
