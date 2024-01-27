#include "trc/drawable/DrawableScene.h"

#include "trc/drawable/AnimationComponent.h"
#include "trc/drawable/DefaultDrawable.h"
#include "trc/drawable/RasterComponent.h"
#include "trc/drawable/RayComponent.h"



namespace trc
{

void DrawableScene::update()
{
    // Update transformations in the node tree
    root.updateAsRoot();
}

auto DrawableScene::getRoot() noexcept -> Node&
{
    return root;
}

auto DrawableScene::getRoot() const noexcept -> const Node&
{
    return root;
}

auto DrawableScene::makeDrawable(const DrawableCreateInfo& info) -> Drawable
{
    const DrawableID id = DrawableComponentScene::makeDrawable();

    std::shared_ptr<DrawableObj> drawable(
        new DrawableObj{ id, *this, info.geo, info.mat },
        [this](DrawableObj* drawable) {
            destroyDrawable(drawable->id);
            delete drawable;
        }
    );

    // Create a rasterization component
    if (info.rasterized)
    {
        auto geo = info.geo.getDeviceDataHandle();

        add<RasterComponent>(id, RasterComponentCreateInfo{
            .geo=info.geo,
            .mat=info.mat,
            .modelMatrixId=drawable->getGlobalTransformID(),
            .anim=geo.hasRig()
                ? add<AnimationComponent>(id, geo.getRig()).engine.getState()
                : AnimationEngine::ID{}
        });
    }

    // Create a ray tracing component
    if (info.rayTraced)
    {
        add<RayComponent>(
            id,
            RayComponentCreateInfo{ info.geo, info.mat, drawable->getGlobalTransformID() }
        );
    }

    return drawable;
}

} // namespace trc
