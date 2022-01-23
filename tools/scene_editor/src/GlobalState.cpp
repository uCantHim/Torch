#include "GlobalState.h"

#include <trc/AssetRegistry.h>

#include "App.h"
#include "Scene.h"
#include "ObjectSelection.h"



PickingState::PickingState(Scene& scene)
    :
    scene(&scene)
{
    trc::on<trc::MouseMoveEvent>([this](auto&) { onMouseMove(); });
    trc::on<trc::MouseClickEvent>([this](auto& e) { onMouseClick(e); });
    trc::on<trc::MouseReleaseEvent>([this](auto& e) { onMouseRelease(e); });
}

void PickingState::hoverObject(SceneObject obj)
{
    hoveredObject = obj;
}

void PickingState::unhoverObject(SceneObject)
{
    hoveredObject = NO_OBJECT;
}

void PickingState::onMouseClick(const vkb::MouseClickEvent& e)
{
    if (e.button == vkb::MouseButton::left && hoveredObject != NO_OBJECT)
    {
        mouseWindowPosAtClick = vkb::Mouse::getPosition();
        selectIsPending = true;
    }
    else if (e.button == vkb::MouseButton::right)
    {
        resetObject();
    }
}

void PickingState::onMouseRelease(const vkb::MouseReleaseEvent& e)
{
    if (e.button == vkb::MouseButton::left)
    {
        if (heldObject == NO_OBJECT)
        {
            global::getObjectSelection().selectObject(hoveredObject);
            selectIsPending = false;
        }
        else {
            dropObject();
        }
    }
}

void PickingState::onMouseMove()
{
    if (selectIsPending)
    {
        float totalMove = glm::distance(mouseWindowPosAtClick, vkb::Mouse::getPosition());
        if (totalMove >= MOUSE_MOVE_DISTANCE_UNTIL_DRAG)
        {
            takeObject(hoveredObject);
            selectIsPending = false;
        }
    }

    // Drag object if one is being held
    if (heldObject != NO_OBJECT)
    {
        moveHeldObject();
    }

    lastFrameMouseWorldPos = App::getTorch().getRenderConfig().getMouseWorldPos(scene->getCamera());
}

void PickingState::takeObject(SceneObject obj)
{
    heldObject = obj;
    initialObjectPos = scene->get<ObjectBaseNode>(obj).getTranslation();
}

void PickingState::dropObject()
{
    heldObject = NO_OBJECT;
}

void PickingState::resetObject()
{
    if (heldObject == NO_OBJECT) return;

    scene->get<ObjectBaseNode>(heldObject).setTranslation(initialObjectPos);
    dropObject();
}

void PickingState::moveHeldObject()
{
    if (heldObject == NO_OBJECT) return;

    const vec3 currentMouseWorldPos = App::getTorch().getRenderConfig().getMouseWorldPos(
        App::getScene().getCamera()
    );

    scene->get<ObjectBaseNode>(heldObject).translate(currentMouseWorldPos - lastFrameMouseWorldPos);
    lastFrameMouseWorldPos = currentMouseWorldPos;
}
