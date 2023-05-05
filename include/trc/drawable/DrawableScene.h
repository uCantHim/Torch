#pragma once

#include <atomic>

#include <trc_util/data/SafeVector.h>

#include "trc/Types.h"
#include "trc/drawable/Drawable.h"
#include "trc/drawable/DrawableComponentScene.h"

namespace trc
{
    /**
     * @brief A shared handle to a drawable object
     *
     * # Example
     * ```cpp
     *
     * Scene scene;
     * Drawable myDrawable = scene.makeDrawable({ myGeo, myMat });
     * ```
     *
     * @note I can extend this to a custom struct with an overloaded `operator->`
     * if a more sophisticated interface is required.
     */
    using Drawable = s_ptr<DrawableObj>;

    struct DrawableCreateInfo
    {
        GeometryID geo;
        MaterialID mat;

        bool rasterized{ true };
        bool rayTraced{ false };
    };

    class DrawableScene
    {
    public:
        explicit DrawableScene(SceneBase& base);

        void updateAnimations(float timeDelta) {
            components.updateAnimations(timeDelta);
        }
        void updateRayData() {
            components.updateRayData();
        }

        auto makeDrawable(const DrawableCreateInfo& createInfo) -> Drawable;

        auto getComponentInternals() -> DrawableComponentScene& {
            return components;
        }

        auto getComponentInternals() const -> const DrawableComponentScene& {
            return components;
        }

    private:
        DrawableComponentScene components;

        util::SafeVector<DrawableObj> drawables;
    };
} // namespace trc
