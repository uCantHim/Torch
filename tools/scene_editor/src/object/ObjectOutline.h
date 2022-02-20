#pragma once

#include <trc/drawable/Drawable.h>

#include "SceneObject.h"

class Scene;

class ObjectOutline
{
public:
    enum class Type
    {
        eHover,
        eSelect
    };

    ObjectOutline(ObjectOutline&&) = default;
    auto operator=(ObjectOutline&&) -> ObjectOutline& = default;

    ObjectOutline(Scene& scene, SceneObject obj, Type outlineType);

private:
    static auto toMaterial(Type type) -> trc::MaterialID;
    static constexpr float OUTLINE_SCALE{ 1.02f };

    u_ptr<trc::Node> node{ new trc::Node };
    trc::UniqueDrawableID drawable;
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
