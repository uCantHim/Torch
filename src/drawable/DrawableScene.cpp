#include "trc/drawable/DrawableScene.h"

#include "trc/drawable/AnimationComponent.h"
#include "trc/drawable/DefaultDrawable.h"
#include "trc/drawable/RasterComponent.h"
#include "trc/drawable/RayComponent.h"



namespace trc
{

DrawableScene::DrawableScene(SceneBase& baseScene)
    :
    components(baseScene)
{
}

auto DrawableScene::makeDrawable(const DrawableCreateInfo& info) -> Drawable
{
    const DrawableID id = components.makeDrawable();

    std::shared_ptr<DrawableObj> drawable(
        &drawables.emplace(ui32{id}, DrawableObj{ id, components, info.geo, info.mat }),
        [this](DrawableObj* drawable) {
            drawables.erase(ui32{drawable->id});
            components.destroyDrawable(drawable->id);
        }
    );

    // Create a rasterization component
    if (info.rasterized)
    {
        auto geo = info.geo.getDeviceDataHandle();

        components.add<RasterComponent>(id, RasterComponentCreateInfo{
            .geo=info.geo,
            .mat=info.mat,
            .modelMatrixId=drawable->getGlobalTransformID(),
            .anim=geo.hasRig()
                ? components.add<AnimationComponent>(id, geo.getRig()).engine.getState()
                : AnimationEngine::ID{}
        });
    }

    // Create a ray tracing component
    if (info.rayTraced)
    {
        components.add<RayComponent>(
            id,
            RayComponentCreateInfo{ info.geo, info.mat, drawable->getGlobalTransformID() }
        );
    }

    return drawable;
}

} // namespace trc
