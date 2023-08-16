#include "trc/drawable/DrawableComponentScene.h"

#include "trc/drawable/AnimationComponent.h"
#include "trc/drawable/RayComponent.h"



namespace trc
{

UniqueDrawableID::UniqueDrawableID(DrawableID drawable, DrawableComponentScene& scene)
    :
    id(drawable),
    scene(&scene)
{
}

UniqueDrawableID::UniqueDrawableID(UniqueDrawableID&& other) noexcept
    :
    id(other.id),
    scene(other.scene)
{
    other.id = DrawableID::NONE;
    other.scene = nullptr;
}

UniqueDrawableID& UniqueDrawableID::operator=(UniqueDrawableID&& other) noexcept
{
    std::swap(id, other.id);
    std::swap(scene, other.scene);
    return *this;
}

UniqueDrawableID::~UniqueDrawableID() noexcept
{
    if (id != DrawableID::NONE && scene != nullptr)
    {
        scene->destroyDrawable(id);
    }
}

} // namespace trc



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
    return rayInstances.size();
}

auto trc::DrawableComponentScene::writeTlasInstances(
    rt::GeometryInstance* instanceBuf,
    const ui32 maxInstances) const
    -> ui32
{
    size_t numInstances = 0;
    for (const auto& inst : get<rt::GeometryInstance>())
    {
        if (numInstances >= maxInstances) {
            break;
        }
        instanceBuf[numInstances] = inst;
        ++numInstances;
    }

    assert(numInstances <= maxInstances);

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
