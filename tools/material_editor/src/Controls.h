#pragma once

#include <trc/Camera.h>
using namespace trc::basic_types;

class MaterialEditorGui;

class MaterialEditorControls
{
public:
    MaterialEditorControls(trc::Window& window, MaterialEditorGui& gui, trc::Camera& camera);

    void update();

private:
};
