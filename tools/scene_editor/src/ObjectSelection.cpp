#include "ObjectSelection.h"



void ObjectSelection::hoverObject(SceneObject obj)
{
    if (hoveredObject != obj)
    {
        unhoverObject();
        hoveredObject = obj;

        std::cout << "Object " << obj << " is now hovered\n";
    }
}

void ObjectSelection::unhoverObject()
{
    hoveredObject = SceneObject::NONE;
}

void ObjectSelection::selectObject(SceneObject obj)
{
    unselectObject();
    selectedObject = obj;
}

void ObjectSelection::unselectObject()
{
    selectedObject = SceneObject::NONE;
}
