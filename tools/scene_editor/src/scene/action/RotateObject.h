#pragma once

#include "Scene.h"
#include "input/Action.h"

namespace action
{
    class RotateObject : public InvertibleAction
    {
    public:
        RotateObject(s_ptr<Scene> scene, SceneObject obj, quat from, quat to)
            : scene(scene), obj(obj), from(from), to(to)
        {}

        void apply() override {
            scene->get<ObjectBaseNode>(obj).setRotation(to);
        }

        void undo() override {
            scene->get<ObjectBaseNode>(obj).setRotation(from);
        }

        s_ptr<Scene> scene;
        const SceneObject obj;
        const quat from;
        const quat to;
    };
} // namespace action
