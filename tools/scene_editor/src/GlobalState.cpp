#include "GlobalState.h"

#include <trc/AssetRegistry.h>

#include "ObjectSelection.h"



const bool PickingState::_init = []
{
    trc::on<trc::MouseMoveEvent>([](const auto&) { onMouseMove(); });
    trc::on<trc::MouseClickEvent>(onMouseClick);
    trc::on<trc::MouseReleaseEvent>(onMouseRelease);

    return true;
}();

void PickingState::hoverObject(SceneObject::ID obj)
{
    hoveredObject = obj;
}

void PickingState::unhoverObject(SceneObject::ID)
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
        if (heldObject == nullptr)
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
            takeObject(App::getScene().getObject(hoveredObject));
            selectIsPending = false;
        }
    }

    // Drag object if one is being held
    if (heldObject != nullptr)
    {
        moveHeldObject();
    }

    lastFrameMouseWorldPos = App::getTorch().getRenderConfig().getMouseWorldPos(
        App::getScene().getCamera()
    );
}

void PickingState::takeObject(SceneObject& obj)
{
    heldObject = &obj;
    initialObjectPos = obj.getSceneNode().getTranslation();
}

void PickingState::dropObject()
{
    heldObject = nullptr;
}

void PickingState::resetObject()
{
    if (heldObject == nullptr) return;

    heldObject->getSceneNode().setTranslation(initialObjectPos);
    dropObject();
}

void PickingState::moveHeldObject()
{
    if (heldObject == nullptr) return;

    const vec3 currentMouseWorldPos = App::getTorch().getRenderConfig().getMouseWorldPos(
        App::getScene().getCamera()
    );

    heldObject->getSceneNode().translate(currentMouseWorldPos - lastFrameMouseWorldPos);
    lastFrameMouseWorldPos = currentMouseWorldPos;
}
