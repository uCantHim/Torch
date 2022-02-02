#pragma once

#include <trc/drawable/Drawable.h>

#include "SceneObject.h"

class Scene;

class ObjectOutline : public trc::Drawable
{
public:
    enum class Type
    {
        eHover,
        eSelect
    };

    ObjectOutline(Scene& scene, SceneObject obj, Type outlineType);

private:
    static auto toMaterial(Type type) -> trc::MaterialID;
    static constexpr float OUTLINE_SCALE{ 1.02f };
};

class ObjectHoverOutline : public ObjectOutline
{
public:
    ObjectHoverOutline(Scene& scene, SceneObject obj);
};

class ObjectSelectOutline : public ObjectOutline
{
public:
    ObjectSelectOutline(Scene& scene, SceneObject obj);
};
