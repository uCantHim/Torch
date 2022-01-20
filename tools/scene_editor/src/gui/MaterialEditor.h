#pragma once

#include <trc/Material.h>

#include "ImGuiUtil.h"

namespace gui
{
    inline void materialEditor(const char* title,
                               trc::Material& mat,
                               auto onSaveCallback,
                               auto onCancelCallback = []() {})
    {
        imGuiTryBegin(title);
        trc::imgui::WindowGuard guard;

        // Color selection
        ig::PushItemWidth(250);
        ig::ColorEdit4("Color", &mat.color.r, ImGuiColorEditFlags_NoAlpha);

        ig::ColorEdit4("Ambient coefficient", &mat.kAmbient.r, ImGuiColorEditFlags_NoAlpha);
        ig::ColorEdit4("Diffuse coefficient", &mat.kDiffuse.r, ImGuiColorEditFlags_NoAlpha);
        ig::ColorEdit4("Specular coefficient", &mat.kSpecular.r, ImGuiColorEditFlags_NoAlpha);

        ig::PushItemWidth(150);
        ig::SliderFloat("Shininess", &mat.shininess, 0.0f, 64.0f);
        ig::SliderFloat("Reflectivity", &mat.reflectivity, 0.0f, 1.0f);

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
} // namespace gui
