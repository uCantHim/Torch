#include "trc/drawable/DrawableScene.h"

#include "trc/drawable/DefaultDrawable.h"



namespace trc
{

DrawableScene::DrawableScene(SceneBase& baseScene)
    :
    components(baseScene)
{
}

auto DrawableScene::makeDrawable(const DrawableCreateInfo& info) -> DrawableHandle
{
    const DrawableID id = components.makeDrawable();

    std::shared_ptr<Drawable> drawable(
        &drawables.emplace(ui32{id}, Drawable{ id, components, info.geo, info.mat }),
        [this](Drawable* drawable) {
            drawables.erase(ui32{drawable->id});
            components.destroyDrawable(drawable->id);
        }
    );

    // Create a rasterization component
    if (info.rasterized)
    {
        AnimationEngine::ID animState{};
        if (auto geo=info.geo.getDeviceDataHandle(); geo.hasRig()) {
            animState = components.makeAnimationEngine(id, geo.getRig()).getState();
        }

        components.makeRasterization(id, RasterComponentCreateInfo{
            .geo=info.geo,
            .mat=info.mat,
            .modelMatrixId=drawable->getGlobalTransformID(),
            .anim=animState,
        });
    }

    // Create a ray tracing component
    if (info.rayTraced) {
        components.makeRaytracing(id, { info.geo, info.mat, drawable->getGlobalTransformID() });
    }

    return drawable;
}

} // namespace trc
