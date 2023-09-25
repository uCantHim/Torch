#pragma once

#include <trc/Scene.h>
#include <trc/TorchRenderConfig.h>
#include <trc/assets/AssetManager.h>
#include <trc/material/FragmentShader.h>

using namespace trc::basic_types;

class MaterialPreview
{
public:
    MaterialPreview(const trc::Instance& instance, const trc::RenderTarget& renderTarget);

    void update();
    auto getDrawConfig() -> trc::DrawConfig;

    void setViewport(ivec2 offset, uvec2 size);
    void setRenderTarget(const trc::RenderTarget& newTarget);

    /**
     * @brief Create and display a new material in the preview
     */
    void makeMaterial(trc::MaterialData data);

private:
    void showMaterial(trc::MaterialID mat);

    trc::AssetManagerBase man;
    trc::TorchRenderConfig config;

    trc::Camera camera;
    trc::Scene scene;

    trc::GeometryID previewGeo;
    trc::MaterialID mat;
    trc::Drawable previewDrawable;
};
