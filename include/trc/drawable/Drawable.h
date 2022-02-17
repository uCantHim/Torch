#pragma once

#include "DrawableComponentScene.h"
#include "DrawablePoolStructs.h"
#include "RasterPipelines.h"

namespace trc
{
    class Drawable : public Node
    {
    public:
        Drawable(const Drawable&) = delete;
        auto operator=(const Drawable&) -> Drawable& = delete;

        Drawable() = default;
        Drawable(Drawable&&) noexcept;
        ~Drawable();

        auto operator=(Drawable&&) noexcept -> Drawable&;

        Drawable(const DrawableCreateInfo& info, DrawableComponentScene& scene);

        /**
         * @brief Create a drawable with a custom pipeline for the g-buffer pass
         */
        Drawable(const DrawableCreateInfo& info,
                 Pipeline::ID gBufferPipeline,
                 DrawableComponentScene& scene);

        /** @brief Legacy constructor */
        Drawable(GeometryID geo, MaterialID material, DrawableComponentScene& scene);

        auto getMaterial() const -> MaterialID;
        auto getGeometry() const -> GeometryID;

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

        /**
         * @brief Remove the Drawable from the scene it is attached to
         */
        void removeFromScene();

    private:
        static void drawShadow(const drawcomp::RasterComponent& data,
                               const DrawEnvironment& env,
                               vk::CommandBuffer cmdBuf);

        static auto makeRasterData(const DrawableCreateInfo& info,
                                   Pipeline::ID gBufferPipeline)
            -> RasterComponentCreateInfo;

        DrawableComponentScene* scene;
        DrawableID id;
        GeometryID geo;
    };
}
