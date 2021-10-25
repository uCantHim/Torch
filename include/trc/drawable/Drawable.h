#pragma once

#include "core/SceneBase.h"
#include "Node.h"
#include "AssetIds.h"
#include "AnimationEngine.h"
#include "DrawablePoolStructs.h"
#include "RasterPipelines.h"

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
        ~Drawable() = default;

        explicit
        Drawable(const DrawableCreateInfo& info);
        Drawable(GeometryID geo, MaterialID material);
        Drawable(GeometryID geo, MaterialID material, SceneBase& scene);

        Drawable(Drawable&&) noexcept = default;
        auto operator=(Drawable&&) noexcept -> Drawable& = default;

        Drawable(const Drawable&) = delete;
        auto operator=(const Drawable&) -> Drawable& = delete;

        auto getMaterial() const -> MaterialID;
        auto getGeometry() const -> GeometryID;

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
        struct DrawableData
        {
            Geometry geo{};
            GeometryID geoId{};
            MaterialID mat{};

            Transformation::ID modelMatrixId;
            AnimationEngine::ID anim;
        };

        static void drawShadow(const DrawableData& data, const DrawEnvironment& env, vk::CommandBuffer cmdBuf);

        Pipeline::ID deferredPipeline;
        SubPass::ID deferredSubpass;
        SceneBase::UniqueRegistrationID deferredRegistration;
        SceneBase::UniqueRegistrationID shadowRegistration;

        AnimationEngine animEngine;
        u_ptr<DrawableData> data;
    };
}
