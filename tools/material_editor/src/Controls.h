#pragma once

#include <optional>

#include <trc/Camera.h>
using namespace trc::basic_types;

#include "GraphScene.h"

class MaterialEditorGui;

struct MaterialEditorControlsCreateInfo
{
    i32 initialZoomLevel{ 0 };
};

class MaterialEditorControls
{
public:
    MaterialEditorControls(trc::Window& window,
                           MaterialEditorGui& gui,
                           trc::Camera& camera,
                           const MaterialEditorControlsCreateInfo& info = {});

    /**
     * @brief Calculate user interaction with the material graph
     *
     * Modifies `GraphScene::interaction` according to user input. Modifies
     * `GraphScene::layout` if nodes have been moved around.
     */
    void update(GraphScene& graph);

private:
    trc::Window* window;
    trc::Camera* camera;

    // Scale camera drag distance by size of the orthogonal camera
    vec2 cameraViewportSize{ 1.0f, 1.0f };
};
