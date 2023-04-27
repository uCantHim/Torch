#pragma once

#include "trc/RasterPipelines.h"
#include "trc/drawable/DrawableComponentScene.h"
#include "trc/drawable/DrawableStructs.h"

namespace trc
{
    class Drawable : public Node
    {
    public:
        Drawable(const Drawable&) = delete;
        auto operator=(const Drawable&) -> Drawable& = delete;
        auto operator=(Drawable&&) noexcept -> Drawable& = delete;

        Drawable(Drawable&&) noexcept = default;
        ~Drawable() noexcept = default;

        auto getGeometry() const -> GeometryID;
        auto getMaterial() const -> MaterialID;

        /**
         * @return bool True if the drawable has a rig and an animation
         *              engine. False otherwise.
         */
        bool isAnimated() const;

        /**
         * @return AnimationEngine& Always returns an animation engine, even
         *                          if the geometry doesn't have a rig.
         * @throw std::out_of_range if the drawable is not animated.
         */
        auto getAnimationEngine() -> AnimationEngine&;

        /**
         * @return AnimationEngine& Always returns an animation engine, even
         *                          if the geometry doesn't have a rig.
         * @throw std::out_of_range if the drawable is not animated.
         */
        auto getAnimationEngine() const -> const AnimationEngine&;

    private:
        friend class DrawableScene;

        Drawable(DrawableID id,
                 DrawableComponentScene& scene,
                 GeometryID geometry,
                 MaterialID material);

        DrawableComponentScene* scene;
        DrawableID id;

        GeometryID geo;
        MaterialID mat;
    };
}
