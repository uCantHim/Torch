#include "drawable/DrawableComponentScene.h"

using namespace trc::drawcomp;

struct RasterRegistrations
{
    std::vector<trc::SceneBase::UniqueRegistrationID> regs;
};



trc::DrawableComponentScene::DrawableComponentScene(SceneBase& base)
    :
    base(&base)
{
}

void trc::DrawableComponentScene::updateAnimations(const float timeDelta)
{
    for (auto& anim : storage.get<AnimationComponent>())
    {
        anim.engine.update(timeDelta);
    }
}

auto trc::DrawableComponentScene::makeDrawable() -> DrawableID
{
    return storage.createObject();
}

void trc::DrawableComponentScene::destroyDrawable(DrawableID drawable)
{
    storage.deleteObject(drawable);
}

void trc::DrawableComponentScene::makeRasterization(
    const DrawableID drawable,
    const RasterComponentCreateInfo& createInfo)
{
    RasterComponent& comp = storage.add<RasterComponent>(drawable, createInfo.drawData);
    RasterRegistrations& reg = storage.add<RasterRegistrations>(drawable);

    for (const auto& f : createInfo.drawFunctions)
    {
        reg.regs.emplace_back(base->registerDrawFunction(
            f.stage, f.subPass, f.pipeline,
            [&comp, func=f.func](auto& env, auto cmdBuf)
            {
                func(comp, env, cmdBuf);
            }
        ).makeUnique());
    }
}

void trc::DrawableComponentScene::makeRaytracing(DrawableID drawable)
{
    throw std::runtime_error("Not implemented");

    storage.add<RayComponent>(drawable);
}

void trc::DrawableComponentScene::makeAnimation(DrawableID drawable, Rig& rig)
{
    storage.add<AnimationComponent>(drawable, rig);
}

void trc::DrawableComponentScene::makeNode(DrawableID drawable)
{
    storage.add<NodeComponent>(drawable);
}

bool trc::DrawableComponentScene::hasRasterization(DrawableID drawable) const
{
    return storage.has<RasterComponent>(drawable);
}

bool trc::DrawableComponentScene::hasRaytracing(DrawableID drawable) const
{
    return storage.has<RayComponent>(drawable);
}

bool trc::DrawableComponentScene::hasAnimation(DrawableID drawable) const
{
    return storage.has<AnimationComponent>(drawable);
}

bool trc::DrawableComponentScene::hasNode(DrawableID drawable) const
{
    return storage.has<NodeComponent>(drawable);
}

auto trc::DrawableComponentScene::getRasterization(DrawableID drawable)
    -> const drawcomp::RasterComponent&
{
    return storage.get<RasterComponent>(drawable);
}

auto trc::DrawableComponentScene::getRaytracing(DrawableID drawable)
    -> const drawcomp::RayComponent&
{
    throw std::runtime_error("Not implemented");

    return storage.get<RayComponent>(drawable);
}

auto trc::DrawableComponentScene::getNode(DrawableID drawable) -> Node&
{
    auto result = storage.tryGet<NodeComponent>(drawable);
    if (result.has_value()) {
        return result.value()->node;
    }

    throw std::out_of_range("[In DrawableComponentScene::getNode]: Drawable does not have a"
                            " node component!");
}

auto trc::DrawableComponentScene::getAnimationEngine(DrawableID drawable) -> AnimationEngine&
{
    auto result = storage.tryGet<AnimationComponent>(drawable);
    if (result.has_value()) {
        return result.value()->engine;
    }

    throw std::out_of_range("[In DrawableComponentScene::getNode]: Drawable does not have an"
                            " animation component!");
}
