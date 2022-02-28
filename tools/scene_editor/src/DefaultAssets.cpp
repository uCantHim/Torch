#include "DefaultAssets.h"

#include <trc/assets/GeneratedGeometry.h>

#include "AssetManager.h"



namespace
{
    g::DefaultGeometries defaultGeos;
    g::DefaultMaterials defaultMats;
}

void initDefaultAssets(AssetManager& am)
{
    static bool _init{ false };
    if (_init) return;
    _init = true;

    defaultGeos.cube = am.add(trc::makeCubeGeo());
    defaultGeos.sphere = am.add(trc::makeSphereGeo());

    defaultMats.undefined = am.add(trc::MaterialDeviceHandle{
        .color=vec4(0.3f, 0.3f, 0.3f, 1.0f),
        .kSpecular=vec4(0.0f)
    });
    defaultMats.objectHighlight = am.add(trc::MaterialDeviceHandle{
        .color=vec4(1.0f),
        .performLighting=false,
    });
    defaultMats.objectSelect = am.add(trc::MaterialDeviceHandle{
        .color=vec4(1.0f, 0.8f, 0.0f, 1.0f),
        .performLighting=false,
    });
}

auto g::geos() -> const DefaultGeometries&
{
    return defaultGeos;
}

auto g::mats() -> const DefaultMaterials&
{
    return defaultMats;
}