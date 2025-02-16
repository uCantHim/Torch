#include "ObjectBrowser.h"

#include <imgui.h>
namespace ig = ImGui;

#include "object/Context.h"



namespace gui
{

ObjectBrowser::ObjectBrowser(s_ptr<Scene> scene)
    :
    ImguiWindow("Scene Object Browser"),
    scene(std::move(scene))
{
}

void ObjectBrowser::drawWindowContent()
{
    for (const auto& meta : scene->iterObjects())
    {
        const SceneObject obj = meta.id;
        const std::string label = obj.toString() + meta.name;

        if (ig::Selectable(label.c_str(), obj == selected))
        {
            selected = obj;
            scene->selectObject(obj);
        }
        if (ig::IsItemHovered()) {
            scene->hoverObject(obj);
        }

        // Right-click context menu
        if (ig::BeginPopupContextItem(label.c_str()))
        {
            drawObjectContextMenu(*scene, obj);
            ig::EndPopup();
        }
    }
}

} // namespace gui
