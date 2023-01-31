#include "ObjectBrowser.h"

#include "App.h"
#include "object/Context.h"



namespace gui
{

ObjectBrowser::ObjectBrowser(MainMenu& menu)
    :
    app(&menu.getApp())
{
}

void ObjectBrowser::drawImGui()
{
    Scene& scene = app->getScene();

    for (const auto& meta : scene.iterObjects())
    {
        const SceneObject obj = meta.id;
        const std::string label = obj.toString() + meta.name;

        if (ig::Selectable(label.c_str(), obj == selected))
        {
            selected = obj;
            scene.selectObject(obj);
        }
        if (ig::IsItemHovered()) {
            scene.hoverObject(obj);
        }

        // Right-click context menu
        if (ig::BeginPopupContextItem(label.c_str()))
        {
            drawObjectContextMenu(scene, obj);
            ig::EndPopup();
        }
    }
}

} // namespace gui
