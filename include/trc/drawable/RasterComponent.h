#pragma once

#include <vector>

#include <componentlib/ComponentStorage.h>

#include "trc/AnimationEngine.h"
#include "trc/Transformation.h"
#include "trc/Types.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/core/SceneBase.h"
#include "trc/drawable/DefaultDrawable.h"
#include "trc/drawable/DrawableComponentScene.h"

namespace trc
{
    struct RasterComponentCreateInfo
    {
        GeometryID geo;
        MaterialID mat;

        Transformation::ID modelMatrixId;
        AnimationEngine::ID anim;
    };

    struct RasterComponent
    {
        /**
         * @brief Sets up the `drawInfo` member
         *
         * Registration of draw functions at a scene occurs in the raster
         * component's `ComponentTraits<>::onCreate` function.
         */
        explicit RasterComponent(const RasterComponentCreateInfo& createInfo);

        // This data is referenced by the draw functions and used at runtime
        // to record draw commands
        s_ptr<DrawableRasterDrawInfo> drawInfo;

        // These handles keep the draw functions alive until the component is
        // destroyed.
        std::vector<trc::SceneBase::UniqueRegistrationID> drawFuncs;
    };
} // namespace trc

template<>
struct componentlib::ComponentTraits<trc::RasterComponent>
{
    void onCreate(trc::DrawableComponentScene& storage,
                  trc::DrawableID drawable,
                  trc::RasterComponent& comp);
};
