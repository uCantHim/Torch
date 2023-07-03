#include "ObjectOutline.h"

#include "asset/DefaultAssets.h"
#include "Scene.h"



ObjectOutline::ObjectOutline(Scene& _scene, SceneObject obj, Type outlineType)
{
    auto& scene = _scene.getDrawableScene();
    auto& d = _scene.get<trc::Drawable>(obj);

    drawable = scene.makeDrawable(trc::DrawableCreateInfo{
        .geo = d->getGeometry(),
        .mat = toMaterial(outlineType),
    });
    drawable->setScale(OUTLINE_SCALE);

    d->attach(*drawable);
}

auto ObjectOutline::toMaterial(Type type) -> trc::MaterialID
{
    switch (type)
    {
        case Type::eHover:  return g::mats().objectHighlight;
        case Type::eSelect: return g::mats().objectSelect;
    }

    throw std::logic_error("Invalid ObjectOutline type");
}



ObjectHoverOutline::ObjectHoverOutline(Scene& scene, SceneObject obj)
    : ObjectOutline(scene, obj, Type::eHover)
{
}

ObjectSelectOutline::ObjectSelectOutline(Scene& scene, SceneObject obj)
    : ObjectOutline(scene, obj, Type::eSelect)
{
}
