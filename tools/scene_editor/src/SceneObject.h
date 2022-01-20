#pragma once

#include <memory>
#include <iostream>

#include <trc/drawable/Drawable.h>
using namespace trc::basic_types;

class SceneObject
{
public:
    using ID = trc::TypesafeID<SceneObject, ui32>;

    /**
     * @param ID id
     * @param std::unique_ptr<DrawableInterface> drawable
     */
    SceneObject(ID id, u_ptr<trc::Drawable> drawable);
    SceneObject(SceneObject&&) noexcept = default;
    ~SceneObject() = default;

    SceneObject(const SceneObject&) = delete;
    SceneObject& operator=(const SceneObject&) = delete;
    SceneObject& operator=(SceneObject&&) noexcept = delete;

    auto getObjectID() const noexcept -> ID;
    auto getSceneNode() noexcept -> trc::Node&;
    auto getSceneNode() const noexcept -> const trc::Node&;
    auto getDrawable() noexcept -> trc::Drawable&;
    auto getDrawable() const noexcept -> const trc::Drawable&;

private:
    const ID id;

    u_ptr<trc::Drawable> drawable;
    trc::Node node;
};
