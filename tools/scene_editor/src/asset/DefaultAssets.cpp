#include "DefaultAssets.h"

#include <trc/Types.h>
#include <trc/assets/import/GeneratedGeometry.h>
using namespace trc::basic_types;



namespace
{
    g::DefaultGeometries defaultGeos;
    g::DefaultMaterials defaultMats;
}

void initDefaultAssets(trc::AssetManager& am)
{
    static bool _init{ false };
    if (_init) return;
    _init = true;

    defaultGeos.cube = am.create(trc::makeCubeGeo());
    defaultGeos.sphere = am.create(trc::makeSphereGeo());

    defaultMats.undefined = am.create(trc::makeMaterial({
        .color=vec3(0.3f, 0.3f, 0.3f),
        .specularCoefficient=0.0f
    }));
    defaultMats.objectHighlight = am.create(trc::makeMaterial({
        .color=vec3(1.0f),
        .emissive=false,
    }));
    defaultMats.objectSelect = am.create(trc::makeMaterial({
        .color=vec3(1.0f, 0.8f, 0.0f),
        .emissive=false,
    }));
}

auto g::geos() -> const DefaultGeometries&
{
    return defaultGeos;
}

auto g::mats() -> const DefaultMaterials&
{
    return defaultMats;
}
