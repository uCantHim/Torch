#pragma once

#include <componentlib/ComponentStorage.h>

#include "trc/Transformation.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/drawable/DrawableComponentScene.h"

namespace trc
{
    struct RayComponentCreateInfo
    {
        GeometryID geo;
        MaterialID mat;

        Transformation::ID transformation;
    };

    struct RayComponent
    {
        /**
         * @throw std::invalid_argument if the geometry in `info` does not have
         *                              an acceleration structure.
         */
        RayComponent(const RayComponentCreateInfo& info);

        Transformation::ID modelMatrix;
        GeometryHandle geo;  // Keep the geometry alive
        ui32 materialIndex;

        ui32 instanceDataIndex;
    };
} // namespace trc

template<>
struct componentlib::ComponentTraits<trc::RayComponent>
{
    void onCreate(trc::DrawableComponentScene& storage,
                  trc::DrawableID drawable,
                  trc::RayComponent& ray);

    void onDelete(trc::DrawableComponentScene& storage,
                  trc::DrawableID /*id*/,
                  trc::RayComponent ray);
};
