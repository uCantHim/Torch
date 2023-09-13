#pragma once

#include <trc/Camera.h>
using namespace trc::basic_types;

#include "GraphScene.h"

class MaterialEditorGui;

class MaterialEditorControls
{
public:
    MaterialEditorControls(trc::Window& window, MaterialEditorGui& gui, trc::Camera& camera);

    void update(GraphScene& graph);

private:
    trc::Window* window;
    trc::Camera* camera;
};
