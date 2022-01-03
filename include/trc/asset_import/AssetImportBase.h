#pragma once

#include <string>
#include <vector>

#include "Types.h"
#include "Geometry.h"
#include "Material.h"

namespace trc
{
    struct Mesh
    {
        std::string name;
        mat4 globalTransform;

        GeometryData geometry;
        std::vector<Material> materials;
        std::optional<RigData> rig;
    };

    /**
     * @brief A holder for all data loaded from a file.
     */
    struct FileImportData
    {
        std::vector<Mesh> meshes;
    };
} // namespace trc
