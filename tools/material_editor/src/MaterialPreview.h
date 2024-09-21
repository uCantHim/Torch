#pragma once

#include <trc/Torch.h>
#include <trc/assets/AssetManager.h>

using namespace trc::basic_types;

class MaterialPreview
{
public:
    MaterialPreview(trc::Window& window, const trc::RenderArea& viewportSize);

    void draw(trc::Frame& frame);

    void setViewport(ivec2 offset, uvec2 size);
    void setRenderTarget(const trc::RenderTarget& newTarget);

    /**
     * @brief Create and display a new material in the preview
     */
    void makeMaterial(trc::MaterialData data);

private:
    void showMaterial(trc::MaterialID mat);

    trc::AssetManager assetManager;
    u_ptr<trc::RenderPipeline> renderPipeline;

    s_ptr<trc::Camera> camera;
    s_ptr<trc::Scene> scene;
    trc::ViewportHandle viewport;

    trc::SunLight sunLight;

    trc::GeometryID previewGeo;
    trc::MaterialID mat;
    trc::Drawable previewDrawable;
};
