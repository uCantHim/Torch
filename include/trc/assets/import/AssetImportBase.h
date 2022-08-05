#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include "Types.h"
#include "assets/Animation.h"
#include "assets/Geometry.h"
#include "assets/Material.h"
#include "assets/Rig.h"

namespace trc
{
    namespace fs = std::filesystem;

    struct ThirdPartyMaterialImport
    {
        std::string name;
        MaterialData data;
    };

    struct ThirdPartyMeshImport
    {
        std::string name;
        mat4 globalTransform;

        GeometryData geometry;
        std::vector<ThirdPartyMaterialImport> materials;
        std::optional<RigData> rig;
        std::vector<AnimationData> animations;
    };

    /**
     * @brief A holder for all data loaded from a file.
     */
    struct ThirdPartyFileImportData
    {
        fs::path filePath;

        std::vector<ThirdPartyMeshImport> meshes;
    };
} // namespace trc
