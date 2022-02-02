#pragma once

#include <trc/AssetIds.h>

class AssetManager;

void initDefaultAssets(AssetManager& assetManager);

namespace g
{
    struct DefaultGeometries
    {
        trc::GeometryID cube;
        trc::GeometryID sphere;
    };

    struct DefaultMaterials
    {
        trc::MaterialID undefined;
        trc::MaterialID objectHighlight;
        trc::MaterialID objectSelect;
    };

    auto geos() -> const DefaultGeometries&;
    auto mats() -> const DefaultMaterials&;
}
