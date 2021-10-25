#include "drawable/DrawablePool.h"

#include "core/Instance.h"
#include "AssetRegistry.h"



trc::DrawablePool::DrawablePool(const ::trc::Instance& instance, const DrawablePoolCreateInfo& info)
    :
    instance(instance),
    drawables(info.maxInstances),
    raster({ info.maxInstances }),
    ray(nullptr)
{
    if (info.initRayTracing && instance.hasRayTracing()) {
        ray.reset(new RayDrawablePool(instance, { info.maxInstances }));
    }
}

trc::DrawablePool::DrawablePool(
    const ::trc::Instance& instance,
    const DrawablePoolCreateInfo& info,
    SceneBase& scene)
    :
    DrawablePool(instance, info)
{
    attachToScene(scene);
}

void trc::DrawablePool::attachToScene(SceneBase& scene)
{
    raster.attachToScene(scene);
}

auto trc::DrawablePool::create(const DrawableCreateInfo& info) -> Handle
{
    return createDrawable(info);
}

void trc::DrawablePool::destroy(Handle instance)
{
    deleteInstance(instance);
}

void trc::DrawablePool::update(vk::CommandBuffer cmdBuf)
{
    if (ray != nullptr) {
        ray->buildTlas(cmdBuf);
    }
}

auto trc::DrawablePool::getRayResources() const
    -> std::pair<vk::AccelerationStructureKHR, vk::Buffer>
{
    if (ray == nullptr)
    {
        throw std::runtime_error("[In DrawablePool::getRayResources]: Ray tracing is not"
                                 " for this pool!");
    }

    return { *ray->getTlas(), ray->getDrawableDataBuffer() };
}

auto trc::DrawablePool::createDrawable(const DrawableCreateInfo& info) -> Handle
{
    const ui32 id = drawableIdPool.generate();
    if (id >= drawables.size())
    {
        throw std::out_of_range(
            "[In DrawablePool::createDrawable]: Unable to create new drawable, pool size"
            " of " + std::to_string(drawables.size()) + " is exhausted"
        );
    }

    auto& d = drawables.at(id);
    assert(d.instances.empty());
    d.geoId = info.geo;
    d.matId = info.mat;
    d.geo = info.geo.get();
    d.isRasterized = info.rasterized;
    d.isRayTraced = info.rayTraced && instance.hasRayTracing();

    if (d.isRasterized) {
        raster.createDrawable(id, info);
    }
    if (d.isRayTraced) {
        ray->createDrawable(id, info);
    }

    return createInstance(id);
}

void trc::DrawablePool::deleteDrawable(ui32 drawableId)
{
    assert(drawables.at(drawableId).instances.empty());

    drawableIdPool.free(drawableId);

    if (drawables.at(drawableId).isRasterized) raster.deleteDrawable(drawableId);
    if (drawables.at(drawableId).isRayTraced) ray->deleteDrawable(drawableId);
}

auto trc::DrawablePool::createInstance(ui32 drawableId) -> Handle
{
    auto& d = drawables.at(drawableId);
    const ui32 id = d.instances.size();

    // Create a user-exposed handle for the new instance
    u_ptr<InstanceHandle> handle(new InstanceHandle(this, drawableId, id));

    // Create animation engine if geometry has a rig
    if (d.geo.hasRig()) {
        handle->animEngine = { *d.geo.getRig() };
    }

    // Create drawable data for the instance
    DrawableInstanceCreateInfo instance{
        handle->getGlobalTransformID(),
        handle->getAnimationEngine().getState()
    };
    if (d.isRasterized) raster.createInstance(drawableId, instance);
    if (d.isRayTraced) ray->createInstance(drawableId, instance);

    return d.instances.emplace_back(std::move(handle)).get();
}

void trc::DrawablePool::deleteInstance(Handle instance)
{
    auto& d = drawables.at(instance->drawableId);

    assert(d.instances.size() > instance->instanceId);

    const ui32 newId = instance->instanceId;
    const ui32 oldId = d.instances.back()->instanceId;

    // Rearrange instance draw data
    std::swap(d.instances.at(newId), d.instances.at(oldId));
    d.instances.at(newId)->instanceId = newId;
    d.instances.pop_back();

    if (d.isRasterized) raster.deleteInstance(instance->drawableId, newId);
    if (d.isRayTraced) ray->deleteInstance(instance->drawableId, newId);

    if (d.instances.empty()) {
        deleteDrawable(instance->drawableId);
    }
}



trc::DrawablePool::InstanceHandle::InstanceHandle(
    DrawablePool* pool,
    ui32 drawDataId,
    ui32 instanceId)
    :
    pool(pool),
    drawableId(drawDataId),
    instanceId(instanceId)
{
}

auto trc::DrawablePool::InstanceHandle::copy() -> Handle
{
    return pool->createInstance(drawableId);
}

void trc::DrawablePool::InstanceHandle::destroy()
{
    pool->deleteInstance(this);
}

auto trc::DrawablePool::InstanceHandle::getGeometry() const -> GeometryID
{
    return pool->drawables.at(drawableId).geoId;
}

auto trc::DrawablePool::InstanceHandle::getMaterial() const -> MaterialID
{
    return pool->drawables.at(drawableId).matId;
}

auto trc::DrawablePool::InstanceHandle::getAnimationEngine() -> AnimationEngine&
{
    return animEngine;
}

auto trc::DrawablePool::InstanceHandle::getAnimationEngine() const -> const AnimationEngine&
{
    return animEngine;
}
