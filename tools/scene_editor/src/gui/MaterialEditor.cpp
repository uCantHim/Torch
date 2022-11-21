#include "MaterialEditor.h"



namespace gui
{

inline void materialEditor(const char* title,
                           trc::MaterialData& mat,
                           auto onSaveCallback,
                           auto onCancelCallback = []() {})
{
    imGuiTryBegin(title);
    trc::imgui::WindowGuard guard;

    // Color selection
    ig::PushItemWidth(250);
    ig::ColorEdit4("Color", &mat.color.r, ImGuiColorEditFlags_NoAlpha);

    ig::PushItemWidth(150);
    ig::SliderFloat("Roughness", &mat.roughness, 0.0f, 1.0f);
    ig::SliderFloat("Metallicness", &mat.metallicness, 0.0f, 1.0f);
    ig::SliderFloat("Specular coefficient", &mat.specularCoefficient, 0.0f, 1.0f);

    ig::NewLine();
    ig::Text("Diffuse texture not implemented");
    ig::Text("Bump texture not implemented");
    ig::Text("Specular texture not implemented");

    ig::NewLine();
    if (ig::Button("save")) {
        onSaveCallback();
    }
    ig::SameLine();
    if (ig::Button("cancel")) {
        onCancelCallback();
    }

    ig::PopItemWidth();
    ig::PopItemWidth();
}

MaterialEditorWindow::MaterialEditorWindow(
    trc::MaterialID material,
    std::function<void(trc::MaterialID, trc::MaterialData)> onSave)
    :
    title("Material Editor (" + material.getMetaData().name + ")"),
    onSave(std::move(onSave)),
    material(material),
    data(material.getModule().getData(material.getDeviceID()))
{
}

bool MaterialEditorWindow::operator()()
{
    if (!windowOpen) return false;

    materialEditor(
        title.c_str(), data,
        [this]{  // On save
            material.getModule().modify(
                material.getDeviceID(),
                [&](auto& mat) { mat = data; }
            );
            onSave(material, data);
            windowOpen = false;
        },
        [this]{ windowOpen = false; }
    );
    return true;
}

} // namespace gui
