#pragma once

#include <componentlib/Table.h>

#include "Assets.h"
#include "Geometry.h"
#include "Material.h"
#include "Transformation.h"
#include "AnimationEngine.h"

namespace trc::drawcomp
{
    struct RasterComponent
    {
        GeometryHandle geo;
        MaterialHandle mat;

        Transformation::ID modelMatrixId;
        AnimationEngine::ID anim;
    };
} // namespace trc::drawcomp

/**
 * Because the data is referenced in draw functions
 */
template<>
struct componentlib::TableTraits<trc::drawcomp::RasterComponent>
{
    using UniqueStorage = std::true_type;
};
