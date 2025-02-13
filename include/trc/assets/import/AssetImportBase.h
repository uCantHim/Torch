#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "trc/Types.h"
#include "trc/assets/Animation.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/assets/Rig.h"

namespace trc
{
    namespace fs = std::filesystem;

    struct ThirdPartyMaterialImport
    {
        std::string name;
        SimpleMaterialData data;
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

        /**
         * @return `nullptr` if no mesh with the specified name was found. A
         *         valid pointer otherwise.
         */
        auto findMesh(std::string_view name) & -> ThirdPartyMeshImport*
        {
            auto it = std::ranges::find_if(meshes, [&](auto& mesh){ return mesh.name == name; });
            if (it != meshes.end()) {
                return &*it;
            }
            return nullptr;
        }
    };
} // namespace trc
