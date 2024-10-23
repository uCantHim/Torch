#include "trc/RaySceneModule.h"

#include "trc/ray_tracing/GeometryUtils.h"



namespace trc
{

auto RaySceneModule::allocateRayInstance(
    RayInstanceData data,
    ui8 mask,
    ui32 shaderBindingTableRecordOffset,
    vk::GeometryInstanceFlagsKHR flags,
    const rt::BLAS& blas) -> ui32
{
    const ui32 index = rayInstanceIdPool.generate();
    rayInstances[index] = data;
    geoInstances.emplace(
        index,
        rt::GeometryInstance{
            mat4{ 1.0f },
            index,
            mask,
            shaderBindingTableRecordOffset,
            flags,
            blas
        }
    );

    return index;
}

void RaySceneModule::freeRayInstance(ui32 index)
{
    rayInstanceIdPool.free(index);
}

void RaySceneModule::setInstanceTransform(ui32 instanceIndex, const mat4& transform)
{
    geoInstances.get(instanceIndex).setTransform(transform);
}

auto RaySceneModule::getMaxRayDeviceDataSize() const -> size_t
{
    return sizeof(RayInstanceData) * rayInstances.size();
}

auto RaySceneModule::getMaxRayGeometryInstances() const -> ui32
{
    return rayInstances.size();
}

auto RaySceneModule::writeTlasInstances(
    rt::GeometryInstance* instanceBuf,
    ui32 maxInstances) const
    -> ui32
{
    size_t numInstances = 0;
    for (const auto& inst : geoInstances)
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

auto RaySceneModule::writeRayDeviceData(
    void* deviceDataBuf,
    size_t maxSize) const
    -> size_t
{
    const size_t size = glm::min(maxSize, rayInstances.size() * sizeof(RayInstanceData));
    memcpy(deviceDataBuf, rayInstances.data(), size);

    return size;
}

} // namespace trc
