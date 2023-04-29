#include "trc/drawable/DrawableComponentScene.h"

#include "trc/drawable/AnimationComponent.h"
#include "trc/drawable/RayComponent.h"



trc::DrawableComponentScene::DrawableComponentScene(SceneBase& base)
    :
    base(&base)
{
}

auto trc::DrawableComponentScene::getSceneBase() -> SceneBase&
{
    assert(base != nullptr);
    return *base;
}

void trc::DrawableComponentScene::updateAnimations(const float timeDelta)
{
    for (auto& anim : get<AnimationComponent>())
    {
        anim.engine.update(timeDelta);
    }
}

void trc::DrawableComponentScene::updateRayData()
{
    const auto join = get<rt::GeometryInstance>().join(get<RayComponent>());
    for (const auto& [_, geoInstance, ray] : join)
    {
        geoInstance.setTransform(ray.modelMatrix.get());
    }
}

auto trc::DrawableComponentScene::getMaxRayDeviceDataSize() const -> size_t
{
    return sizeof(RayInstanceData) * rayInstances.size();
}

auto trc::DrawableComponentScene::getMaxRayGeometryInstances() const -> ui32
{
    return get<rt::GeometryInstance>().size();
}

auto trc::DrawableComponentScene::writeTlasInstances(
    rt::GeometryInstance* instanceBuf,
    const ui32 maxInstances) const
    -> ui32
{
    const size_t numInstances = glm::min(size_t{maxInstances},
                                         get<rt::GeometryInstance>().size());
    const size_t size = numInstances * sizeof(rt::GeometryInstance);
    memcpy(instanceBuf, get<rt::GeometryInstance>().data(), size);

    return numInstances;
}

auto trc::DrawableComponentScene::writeRayDeviceData(
    void* deviceDataBuf,
    size_t maxSize) const
    -> size_t
{
    const size_t size = glm::min(maxSize, rayInstances.size() * sizeof(RayInstanceData));
    memcpy(deviceDataBuf, rayInstances.data(), size);

    return size;
}

auto trc::DrawableComponentScene::allocateRayInstance(RayInstanceData data) -> ui32
{
    const ui32 index = rayInstanceIdPool.generate();
    rayInstances[index] = data;
    return index;
}

void trc::DrawableComponentScene::freeRayInstance(ui32 index)
{
    rayInstanceIdPool.free(index);
}
