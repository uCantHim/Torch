#pragma once

#include "core/SceneBase.h"
#include "Node.h"
#include "Geometry.h"
#include "AnimationEngine.h"
#include "AssetIds.h"
#include "DrawableData.h"

namespace trc::legacy
{
    /**
     * The animation engine, if the geometry has a rig, is not updated
     * automatically! The user has to take care of that.
     */
    class Drawable : public Node
    {
    public:
        Drawable() = default;
        Drawable(GeometryID geo, MaterialID material);
        Drawable(GeometryID geo, MaterialID material, SceneBase& scene);
        ~Drawable();

        Drawable(Drawable&&) noexcept;
        auto operator=(Drawable&&) noexcept -> Drawable&;

        Drawable(const Drawable&) = delete;
        auto operator=(const Drawable&) -> Drawable& = delete;

        auto getMaterial() const -> MaterialID;
        auto getGeometry() const -> GeometryID;

        void setMaterial(MaterialID matIndex);
        void setGeometry(GeometryID newGeo);

        /**
         * @return AnimationEngine& Always returns an animation engine, even
         *                          if the geometry doesn't have a rig.
         */
        auto getAnimationEngine() noexcept -> AnimationEngine&;

        /**
         * @return AnimationEngine& Always returns an animation engine, even
         *                          if the geometry doesn't have a rig.
         */
        auto getAnimationEngine() const noexcept -> const AnimationEngine&;

        void enableTransparency();

        /**
         * @brief Register all necessary functions at a scene
         *
         * Can only be attached to one scene at a time. If the drawable is
         * already attached to a scene, the drawable is removed from that
         * scene first.
         *
         * @param SceneBase& scene The scene to attach to.
         */
        void attachToScene(SceneBase& scene);

        /**
         * @brief Remove the Drawable from the scene it is currently
         *        attached to
         */
        void removeFromScene();

    private:
        void updateDrawFunctions();

        static void drawShadow(DrawableData* data, const DrawEnvironment& env, vk::CommandBuffer cmdBuf);

        SceneBase* currentScene{ nullptr };
        SceneBase::UniqueRegistrationID deferredRegistration;
        SceneBase::UniqueRegistrationID shadowRegistration;

        GeometryID geoIndex;
        AnimationEngine animEngine;

        ui32 drawableDataId{ DrawableDataStore::create(*this, animEngine) };
        DrawableData* data{ &DrawableDataStore::get(drawableDataId) };
    };
}
