#pragma once

#include <optional>

#include "trc/AnimationEngine.h"
#include "trc/Node.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/drawable/DrawableComponentScene.h"

namespace trc
{
    class DrawableObj : public Node
    {
    public:
        DrawableObj(const DrawableObj&) = delete;
        auto operator=(const DrawableObj&) -> DrawableObj& = delete;
        auto operator=(DrawableObj&&) noexcept -> DrawableObj& = delete;

        DrawableObj(DrawableObj&&) noexcept = default;
        ~DrawableObj() noexcept = default;

        auto getGeometry() const -> GeometryID;
        auto getMaterial() const -> MaterialID;

        /**
         * @return bool True if the drawable has an animation engine.
         *              False otherwise.
         */
        bool isAnimated() const;

        /**
         * @return std::optional<AnimationEngine*>
         */
        auto getAnimationEngine() -> std::optional<AnimationEngine*>;

    private:
        friend class DrawableScene;

        DrawableObj(DrawableID id,
                    DrawableComponentScene& scene,
                    GeometryID geometry,
                    MaterialID material);

        DrawableComponentScene* scene;
        DrawableID id;

        GeometryID geo;
        MaterialID mat;
    };
}
