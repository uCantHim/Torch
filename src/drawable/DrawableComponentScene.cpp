#include "trc/drawable/DrawableComponentScene.h"

using namespace trc::drawcomp;



struct RasterRegistrations
{
    std::vector<trc::SceneBase::UniqueRegistrationID> regs;
};



trc::UniqueDrawableID::UniqueDrawableID(DrawableComponentScene& scene, DrawableID id)
    :
    scene(&scene),
    id(id)
{
}

trc::UniqueDrawableID::UniqueDrawableID(UniqueDrawableID&& other) noexcept
    :
    scene(other.scene),
    id(other.id)
{
    other.scene = nullptr;
    other.id = DrawableID::NONE;
}

auto trc::UniqueDrawableID::operator=(UniqueDrawableID&& other) noexcept -> UniqueDrawableID&
{
    this->scene = other.scene;
    this->id = other.id;
    other.scene = nullptr;
    other.id = DrawableID::NONE;

    return *this;
}

trc::UniqueDrawableID::~UniqueDrawableID() noexcept
{
    if (id != DrawableID::NONE && scene != nullptr)
    {
        scene->destroyDrawable(id);
    }
}

trc::UniqueDrawableID::operator bool() const
{
    return id != DrawableID::NONE;
}

trc::UniqueDrawableID::operator trc::DrawableID() const
{
    return id;
}

auto trc::UniqueDrawableID::get() const -> DrawableID
{
    return id;
}



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

auto trc::DrawableComponentScene::writeTlasInstances(rt::GeometryInstance* instanceBuf) const
    -> size_t
{
    for (ui32 i = 0; const auto& ray : storage.get<RayComponent>())
    {
        instanceBuf[i] = rt::GeometryInstance(
            ray.modelMatrixId.get(), ray.drawableBufferIndex,
            0xff, 0,
            vk::GeometryInstanceFlagBitsKHR::eForceOpaque
            | vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
            ray.geo.getAccelerationStructure()
        );
        ++i;
    }

    return storage.get<RayComponent>().size();
}

auto trc::DrawableComponentScene::getRaySceneData() const -> const std::vector<DrawableRayData>&
{
    return drawableData;
}

auto trc::DrawableComponentScene::makeDrawable() -> DrawableID
{
    return storage.createObject();
}

auto trc::DrawableComponentScene::makeDrawableUnique() -> UniqueDrawableID
{
    return { *this, storage.createObject() };
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

void trc::DrawableComponentScene::makeRaytracing(
    DrawableID drawable,
    const RayComponentCreateInfo& createInfo)
{
    auto geo = createInfo.geo;
    if (!geo.getDeviceDataHandle().hasAccelerationStructure()) {
        geo.getModule().makeAccelerationStructure(geo.getDeviceID());
    }

    auto& ray = storage.add<RayComponent>(drawable, RayComponent{
        .geo = createInfo.geo.getDeviceDataHandle(),
        .modelMatrixId = createInfo.transformation
    });

    if (ray.drawableBufferIndex >= drawableData.size()) {
        drawableData.resize(ray.drawableBufferIndex + 1);
    }
    drawableData[ray.drawableBufferIndex] = DrawableRayData{
        .geometryIndex=createInfo.geo.getDeviceDataHandle().getDeviceIndex(),
        .materialIndex=createInfo.mat.getDeviceDataHandle().getBufferIndex()
    };
}

void trc::DrawableComponentScene::makeAnimationEngine(DrawableID drawable, RigHandle rig)
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

    throw std::out_of_range("[In DrawableComponentScene::getAnimationEngine]: Drawable does not"
                            " have an animation component!");
}
