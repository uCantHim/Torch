#include "MaterialPreview.h"

#include <trc/assets/SimpleMaterial.h>
#include <trc/assets/import/GeneratedGeometry.h>



MaterialPreview::MaterialPreview(
    const trc::Instance& instance,
    const trc::RenderTarget& renderTarget)
    :
    config(
        instance,
        trc::TorchRenderConfigCreateInfo{
            renderTarget,
            &man.getDeviceRegistry(),
            trc::makeDefaultAssetModules(
                instance, man.getDeviceRegistry(),
                { .maxGeometries=1, .maxTextures=20, .maxFonts=1 }
            ),
            1,
            false,
            [&]{ return trc::Mouse::getPosition(); },
        }
    ),
    previewGeo(man.create<trc::Geometry>(std::make_unique<trc::InMemorySource<trc::Geometry>>(
        trc::makeSphereGeo()
    ))),
    mat(man.create<trc::Material>(std::make_unique<trc::InMemorySource<trc::Material>>(
        trc::makeMaterial({ .color{ 1.0f, 0.8f, 0.2f } })
    )))
{
    config.setClearColor({ 0.4f, 0.0f, 1.0f, 1.0f });

    // Create scene
    scene.getLights().makeSunLight(vec3(1.0f), vec3(-1, -1, -1), 0.6f);
    camera.makePerspective(1.0f, 45.0f, 0.01f, 10.0f);
    camera.lookAt(vec3(0, 0, 5), vec3(0.0f), vec3(0, 1, 0));

    // Create assets
    showMaterial(mat);
}

void MaterialPreview::update()
{
    config.perFrameUpdate(camera, scene);
}

auto MaterialPreview::getDrawConfig() -> trc::DrawConfig
{
    return { scene, config };
}

void MaterialPreview::setViewport(ivec2 offset, uvec2 size)
{
    config.setViewport(offset, size);
}

void MaterialPreview::setRenderTarget(const trc::RenderTarget& newTarget)
{
    config.setRenderTarget(newTarget);
}

void MaterialPreview::makeMaterial(trc::MaterialData data)
{
    man.destroy(mat);
    mat = man.create<trc::Material>(
        std::make_unique<trc::InMemorySource<trc::Material>>(std::move(data))
    );

    showMaterial(mat);
}

void MaterialPreview::showMaterial(trc::MaterialID mat)
{
    previewDrawable = scene.makeDrawable({ previewGeo, mat, true, false });
}
