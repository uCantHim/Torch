#pragma once

#include "Scene.h"
#include "input/Action.h"

namespace action
{
    class MoveObject : public InvertibleAction
    {
    public:
        MoveObject(s_ptr<Scene> scene, SceneObject obj, vec3 from, vec3 to)
            : scene(scene), obj(obj), from(from), to(to)
        {}

        void apply() override {
            scene->get<ObjectBaseNode>(obj).setTranslation(to);
        }

        void undo() override {
            scene->get<ObjectBaseNode>(obj).setTranslation(from);
        }

        s_ptr<Scene> scene;
        const SceneObject obj;
        const vec3 from;
        const vec3 to;
    };
} // namespace action
