#pragma once

#include <optional>
#include <stack>

#include <trc/Camera.h>
using namespace trc::basic_types;

#include "GraphManipulator.h"
#include "GraphScene.h"

class ControlState;
class MaterialEditorGui;

struct MaterialEditorControlsCreateInfo
{
    i32 initialZoomLevel{ 0 };
};

class MaterialEditorControls
{
public:
    MaterialEditorControls(const MaterialEditorControls&) = delete;
    MaterialEditorControls(MaterialEditorControls&&) noexcept = delete;
    MaterialEditorControls& operator=(const MaterialEditorControls&) = delete;
    MaterialEditorControls& operator=(MaterialEditorControls&&) noexcept = delete;

    MaterialEditorControls(trc::Window& window,
                           MaterialEditorGui& gui,
                           trc::Camera& camera,
                           const MaterialEditorControlsCreateInfo& info = {});

    // Need to declare this because otherwise u_ptr<ControlState>::~u_ptr would
    // be required, but ControlState is incomplete.
    ~MaterialEditorControls() noexcept;

    /**
     * @brief Calculate user interaction with the material graph
     *
     * Modifies `GraphScene::interaction` according to user input. Modifies
     * `GraphScene::layout` if nodes have been moved around.
     */
    void update(GraphScene& graph, GraphManipulator& manip);

private:
    trc::Window* window;
    trc::Camera* camera;
    MaterialEditorGui* gui;

    vec2 cameraViewportSize{ 1.0f, 1.0f };

    std::stack<u_ptr<ControlState>> stateStack;
};
