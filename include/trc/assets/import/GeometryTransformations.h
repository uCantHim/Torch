#pragma once

#include "trc/assets/Geometry.h"

namespace trc
{
    /**
     * @brief Compute tangents for a geometry
     *
     * Calculate tangents for a geometry from vertex positions, normals,
     * and UV coordinates.
     *
     * The geometry must be triangulated.
     *
     * @param [in/out] GeometryData& result The tangent members of all
     *        vertices will be overwritten with the computed values.
     */
    void computeTangents(GeometryData& result);
} // namespace trc
