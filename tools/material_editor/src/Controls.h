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
    auto toWorldPos(vec2 screenPos) const -> vec2;
    auto toWorldDir(vec2 screenPos) const -> vec2;

    /**
     * @param bool append If true, add selected nodes to the current selection.
     *                    Otherwise overwrite the current selection with nodes
     *                    in the selection area.
     */
    void selectNodesInArea(Hitbox area, GraphScene& graph, bool append);

    trc::Window* window;
    trc::Camera* camera;
    MaterialEditorGui* gui;

    // Scale camera drag distance by size of the orthogonal camera
    vec2 cameraViewportSize{ 1.0f, 1.0f };
};
