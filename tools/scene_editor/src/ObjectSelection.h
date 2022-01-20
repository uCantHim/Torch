#pragma once

#include "SceneObject.h"

namespace global
{
    /**
     * Global state that stores selected objects
     */
    class ObjectSelection
    {
    public:
        void selectObject(SceneObject::ID obj)
        {
            unselectObject();
            selectedObject = obj;
            std::cout << "Object selected\n";
        }

        void unselectObject()
        {

        }

    private:
        SceneObject::ID selectedObject;
    };

    static inline auto getObjectSelection() -> ObjectSelection&
    {
        static ObjectSelection objectSelection;
        return objectSelection;
    }
} // namespace global
