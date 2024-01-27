#pragma once

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

    class DrawableScene : public DrawableComponentScene
    {
    public:
        auto makeDrawable(const DrawableCreateInfo& createInfo) -> Drawable;
    };
} // namespace trc
