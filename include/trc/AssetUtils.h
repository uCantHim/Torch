#pragma once

#include <memory>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

#include "utils/FBXLoader.h"
#include "AssetRegistry.h"
#include "Scene.h"
#include "Drawable.h"

namespace trc
{
    struct SceneImportResult
    {
        Scene scene;
        std::vector<std::unique_ptr<Drawable>> drawables;

        std::vector<std::pair<GeometryID, MaterialID>> importedGeometries;
    };

#ifdef TRC_USE_FBX_SDK
    extern auto loadScene(const fs::path& fbxFilePath) -> SceneImportResult;
#endif
}
