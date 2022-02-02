#include "ObjectSelection.h"

#include "Scene.h"
#include "ObjectOutline.h"



ObjectSelection::ObjectSelection(Scene& scene)
    :
    scene(&scene)
{
}

void ObjectSelection::hoverObject(SceneObject obj)
{
    if (hoveredObject != obj)
    {
        unhoverObject();
        hoveredObject = obj;

        if (obj != SceneObject::NONE)
        {
            scene->add<ObjectHoverOutline>(obj, *scene, obj);
        }
    }
}

void ObjectSelection::unhoverObject()
{
    if (hoveredObject != SceneObject::NONE) {
        scene->remove<ObjectHoverOutline>(hoveredObject);
    }

    hoveredObject = SceneObject::NONE;
}

auto ObjectSelection::getHoveredObject() const -> SceneObject
{
    return hoveredObject;
}

bool ObjectSelection::hasHoveredObject() const
{
    return getHoveredObject() != SceneObject::NONE;
}

void ObjectSelection::selectObject(SceneObject obj)
{
    unselectObject();
    selectedObject = obj;

    if (obj != SceneObject::NONE)
    {
        scene->add<ObjectSelectOutline>(obj, *scene, obj);
    }
}

void ObjectSelection::unselectObject()
{
    if (selectedObject != SceneObject::NONE) {
        scene->remove<ObjectSelectOutline>(selectedObject);
    }

    selectedObject = SceneObject::NONE;
}

auto ObjectSelection::getSelectedObject() const -> SceneObject
{
    return selectedObject;
}

bool ObjectSelection::hasSelectedObject() const
{
    return getSelectedObject() != SceneObject::NONE;
}
