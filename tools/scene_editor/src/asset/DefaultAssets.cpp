#include "DefaultAssets.h"

#include <optional>

#include <trc/Types.h>
#include <trc/assets/import/GeneratedGeometry.h>
using namespace trc::basic_types;



namespace
{
    std::optional<g::DefaultGeometries> defaultGeos;
    std::optional<g::DefaultMaterials> defaultMats;
}

void initDefaultAssets(trc::AssetManager& am)
{
    static bool _init{ false };
    if (_init) return;
    _init = true;

    defaultGeos = {
        .cube = am.create(trc::makeCubeGeo()),
        .sphere = am.create(trc::makeSphereGeo()),
    };

    defaultMats = {
        .undefined = am.create(trc::makeMaterial({
            .color=vec3(0.3f, 0.3f, 0.3f),
            .specularCoefficient=0.0f
        })),
        .objectHighlight = am.create([]{
            auto mat = trc::makeMaterial({
                .color=vec3(1.0f),
                .emissive=true,
            });
            mat.depthBiasConstantFactor = 3.0f;
            mat.depthBiasSlopeFactor = 3.0f;
            mat.cullMode = vk::CullModeFlagBits::eFront;
            mat.depthWrite = false;

            return mat;
        }()),
        .objectSelect = am.create([]{
            auto mat = trc::makeMaterial({
                .color=vec3(1.0f, 0.8f, 0.0f),
                .emissive=true,
            });
            mat.depthBiasConstantFactor = 3.0f;
            mat.depthBiasSlopeFactor = 3.0f;
            mat.cullMode = vk::CullModeFlagBits::eFront;
            mat.depthWrite = false;

            return mat;
        }()),
        .objectHitbox = am.create([]{
            auto mat = trc::makeMaterial({
                .color=vec3(1.0f, 0.8f, 0.0f),
                .emissive=true,
            });
            mat.polygonMode = vk::PolygonMode::eLine;

            return mat;
        }()),
    };
}

auto g::geos() -> const DefaultGeometries&
{
    if (!defaultGeos.has_value()) {
        throw std::runtime_error("Unable to access default geometries: Must be initialized with"
                                 " `initDefaultAssets` first!");
    }
    return defaultGeos.value();
}

auto g::mats() -> const DefaultMaterials&
{
    if (!defaultMats.has_value()) {
        throw std::runtime_error("Unable to access default materials: Must be initialized with"
                                 " `initDefaultAssets` first!");
    }
    return defaultMats.value();
}
