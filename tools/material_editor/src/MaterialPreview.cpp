#include "MaterialPreview.h"

#include <trc/assets/SimpleMaterial.h>
#include <trc/assets/import/GeneratedGeometry.h>
#include <trc/util/NullDataStorage.h>



MaterialPreview::MaterialPreview(
    trc::Window& window,
    const trc::RenderArea& viewportSize)
    :
    assetManager(std::make_shared<trc::NullDataStorage>()),
    renderPipeline(trc::makeTorchRenderPipeline(
        window.getInstance(),
        window,
        trc::TorchPipelineCreateInfo{
            1,  // max viewports
            assetManager.getDeviceRegistry(),
            { .maxGeometries=1, .maxTextures=20, .maxFonts=1 },
            1,  // shadow map
            1,  // transparent frags
            false, 0  // ray tracing
        }
    )),
    camera(std::make_shared<trc::Camera>()),
    scene(std::make_shared<trc::Scene>()),
    viewport(renderPipeline->makeViewport(
        viewportSize,
        camera,
        scene,
        { 0.4f, 0.0f, 1.0f, 1.0f }
    )),
    previewGeo(assetManager.create<trc::Geometry>(
        trc::makeSphereGeo()
    )),
    mat(assetManager.create<trc::Material>(std::make_unique<trc::InMemorySource<trc::Material>>(
        trc::makeMaterial({ .color{ 1.0f, 0.8f, 0.2f } })
    )))
{
    window.addCallbackAfterSwapchainRecreate([this](auto& sc) {
        renderPipeline->changeRenderTarget(trc::makeRenderTarget(sc));
    });

    // Create scene
    sunLight = scene->getLights().makeSunLight(vec3(1.0f), vec3(-1, -1, -1), 0.6f);
    camera->makePerspective(1.0f, 45.0f, 0.01f, 10.0f);
    camera->lookAt(vec3(0, 0, 5), vec3(0.0f), vec3(0, 1, 0));

    // Create assets
    showMaterial(mat);
}

void MaterialPreview::draw(trc::Frame& frame)
{
    renderPipeline->drawAllViewports(frame);
}

void MaterialPreview::setViewport(ivec2 offset, uvec2 size)
{
    viewport->resize({ offset, size });
}

void MaterialPreview::makeMaterial(trc::MaterialData data)
{
    assetManager.destroy(mat);
    mat = assetManager.create<trc::Material>(std::move(data));

    showMaterial(mat);
}

void MaterialPreview::showMaterial(trc::MaterialID mat)
{
    previewDrawable = scene->makeDrawable({ previewGeo, mat, true, false });
}
