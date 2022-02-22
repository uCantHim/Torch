#pragma once

#include "SceneObject.h"

class Scene;

/**
 * Global state that stores selected objects
 */
class ObjectSelection
{
public:
    explicit ObjectSelection(Scene& scene);

    void hoverObject(SceneObject obj);
    void unhoverObject();
    auto getHoveredObject() const -> SceneObject;
    bool hasHoveredObject() const;

    void selectObject(SceneObject obj);
    void unselectObject();
    auto getSelectedObject() const -> SceneObject;
    bool hasSelectedObject() const;

private:
    Scene* scene;

    SceneObject hoveredObject{ SceneObject::NONE };
    SceneObject selectedObject{ SceneObject::NONE };
};
