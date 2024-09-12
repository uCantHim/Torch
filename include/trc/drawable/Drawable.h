#pragma once

#include <optional>

#include <componentlib/ComponentID.h>

#include "trc/AnimationEngine.h"
#include "trc/Node.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"

namespace trc
{
    class DrawableScene;

    namespace drawcomp {
        struct _DrawableIdTypeTag {};
    }
    using DrawableID = componentlib::ComponentID<drawcomp::_DrawableIdTypeTag>;

    class DrawableObj : public Node
    {
    public:
        DrawableObj(const DrawableObj&) = delete;
        auto operator=(const DrawableObj&) -> DrawableObj& = delete;
        auto operator=(DrawableObj&&) noexcept -> DrawableObj& = delete;

        DrawableObj(DrawableObj&&) noexcept = default;
        ~DrawableObj() noexcept override = default;

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
                    DrawableScene& scene,
                    GeometryID geometry,
                    MaterialID material);

        DrawableScene* scene;
        DrawableID id;

        GeometryID geo;
        MaterialID mat;
    };
}
