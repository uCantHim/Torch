#pragma once

#include "base/SceneBase.h"
#include "Node.h"
#include "PipelineDefinitions.h"
#include "Geometry.h"
#include "AnimationEngine.h"
#include "PickableRegistry.h"
#include "AssetIds.h"
#include "DrawableData.h"

namespace trc
{
    class Drawable : public Node
    {
    public:
        Drawable() = default;
        explicit
        Drawable(GeometryID geo, MaterialID material = MaterialID(0));
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

        /**
         * @brief Enable picking for the drawable
         *
         * In order to enable picking, a Pickable needs to be created and
         * associated with the Drawable. That is what this function does.
         *
         * An object of type PickableType is created at the
         * PickableRegistry. The provided PickableType must derive from
         * Pickable for this to work. The ID of the created Pickable is
         * stored in the Drawable.
         *
         * The pickable object is destroyed when the drawable is destroyed.
         *
         * Picking cannot be disabled once it has been enabled.
         *
         * @tparam PickableType Type of the Pickable that's created for the
         *                      Drawable.
         * @tparam ...Args Argument types for the constructor of the
         *                 pickable. Can be deduced from the function
         *                 arguments.
         * @param Args&&... args Constructor arguments for the pickable.
         */
        template<typename PickableType, typename ...Args>
        auto enablePicking(Args&&... args) -> PickableType&
        {
            PickableType& newPickable = PickableRegistry::makePickable<PickableType, Args...>(
                std::forward<Args>(std::move(args))...
            );

            data->pickableId = newPickable.getPickableId();
            updateDrawFunctions();

            return newPickable;
        }

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

        ui32 drawableDataId{ DrawableDataStore::create(getGlobalTransformMatId()) };
        DrawableData* data{ &DrawableDataStore::get(drawableDataId) };
        GeometryID geoIndex{ 0 };
    };
}
