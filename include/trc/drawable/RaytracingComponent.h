#pragma once

#include <componentlib/Table.h>

#include "AssetIds.h"
#include "Transformation.h"
#include "AnimationEngine.h"
#include "ray_tracing/AccelerationStructure.h"

namespace trc::drawcomp
{
    struct RayComponent
    {
        u_ptr<rt::BLAS> blas;  // TODO: Store BLAS in AssetRegistry and reference them here

        GeometryID geo;
        MaterialID mat;

        Transformation::ID modelMatrixId;
        AnimationEngine::ID anim;
    };
} // namespace trc::drawcomp



template<>
struct componentlib::TableTraits<trc::drawcomp::RayComponent>
{
    using UniqueStorage = std::true_type;
};
