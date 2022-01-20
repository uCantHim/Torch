#pragma once

#include <glm/glm.hpp>
using namespace glm;
#include <vkb/event/EventHandler.h>
#include <vkb/event/InputEvents.h>

#include "App.h"
#include "SceneObject.h"

class PickingState
{
public:
    static void hoverObject(SceneObject::ID obj);
    static void unhoverObject(SceneObject::ID obj);
    static auto getHoveredObject() -> SceneObject::ID;

private:
    static constexpr SceneObject::ID NO_OBJECT{ SceneObject::ID::NONE };
    static constexpr float MOUSE_MOVE_DISTANCE_UNTIL_DRAG{ 15 };
    static constexpr float MOUSE_WORLD_POS_PLANE{ 0.95f };
    static const bool _init;

    static void onMouseClick(const vkb::MouseClickEvent& e);
    static void onMouseRelease(const vkb::MouseReleaseEvent& e);
    static void onMouseMove();

    static inline bool selectIsPending{ false };
    // Mouse position when a click event occurs. The clicked object is
    // moved instead of selected if mouse moves far enough during click.
    static inline vec2 mouseWindowPosAtClick;

    static void takeObject(SceneObject& obj);
    static void dropObject();
    static void resetObject();
    static void moveHeldObject();

    static inline SceneObject::ID hoveredObject;
    static inline SceneObject* heldObject{ nullptr };
    static inline vec3 lastFrameMouseWorldPos;
    static inline vec3 initialObjectPos;
};
