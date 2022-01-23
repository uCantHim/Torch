#pragma once

#include <glm/glm.hpp>
using glm::vec2;
using glm::vec3;
#include <vkb/event/EventHandler.h>
#include <vkb/event/InputEvents.h>

#include "SceneObject.h"

class Scene;

class PickingState
{
public:
    PickingState(Scene& scene);

    void hoverObject(SceneObject obj);
    void unhoverObject(SceneObject obj);
    auto getHoveredObject() -> SceneObject;

private:
    static constexpr SceneObject NO_OBJECT{ SceneObject::NONE };
    static constexpr float MOUSE_MOVE_DISTANCE_UNTIL_DRAG{ 15 };
    static constexpr float MOUSE_WORLD_POS_PLANE{ 0.95f };

    Scene* scene;

    void onMouseClick(const vkb::MouseClickEvent& e);
    void onMouseRelease(const vkb::MouseReleaseEvent& e);
    void onMouseMove();

    bool selectIsPending{ false };
    // Mouse position when a click event occurs. The clicked object is
    // moved instead of selected if mouse moves far enough during click.
    vec2 mouseWindowPosAtClick;

    void takeObject(SceneObject obj);
    void dropObject();
    void resetObject();
    void moveHeldObject();

    SceneObject hoveredObject;
    SceneObject heldObject;
    vec3 lastFrameMouseWorldPos;
    vec3 initialObjectPos;
};
