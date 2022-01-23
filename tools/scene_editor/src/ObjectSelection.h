#pragma once

#include "SceneObject.h"

/**
 * Global state that stores selected objects
 */
class ObjectSelection
{
public:
    void hoverObject(SceneObject obj);
    void unhoverObject();

    void selectObject(SceneObject obj);
    void unselectObject();

private:
    SceneObject hoveredObject{ SceneObject::NONE };
    SceneObject selectedObject{ SceneObject::NONE };
};
