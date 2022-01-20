#include "SceneObject.h"

#include "GlobalState.h"



SceneObject::SceneObject(ID id, u_ptr<trc::Drawable> d)
    :
    id(id),
    drawable(std::move(d))
{
    assert(drawable != nullptr);

    node.attach(*drawable);
}

auto SceneObject::getObjectID() const noexcept -> ID
{
    return id;
}

auto SceneObject::getSceneNode() noexcept -> trc::Node&
{
    return node;
}

auto SceneObject::getSceneNode() const noexcept -> const trc::Node&
{
    return node;
}

auto SceneObject::getDrawable() noexcept -> trc::Drawable&
{
    return *drawable;
}

auto SceneObject::getDrawable() const noexcept -> const trc::Drawable&
{
    return *drawable;
}
