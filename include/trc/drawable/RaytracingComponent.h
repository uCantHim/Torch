#pragma once

#include <componentlib/ComponentStorage.h>

#include "trc/AnimationEngine.h"
#include "trc/Transformation.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc_util/data/ObjectId.h"

namespace trc::drawcomp
{
    struct RayComponent
    {
        GeometryHandle geo;
        Transformation::ID modelMatrixId;

        ui32 drawableBufferIndex{ static_cast<ui32>(bufferIndexPool.generate()) };

        static inline data::IdPool bufferIndexPool;
    };
} // namespace trc::drawcomp

template<>
struct componentlib::ComponentTraits<trc::drawcomp::RayComponent>
{
    void onDelete(auto& /*storage*/, auto /*id*/, trc::drawcomp::RayComponent ray)
    {
        trc::drawcomp::RayComponent::bufferIndexPool.free(ray.drawableBufferIndex);
    }
};
