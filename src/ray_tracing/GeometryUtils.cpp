#include "trc/ray_tracing/GeometryUtils.h"

#include "trc/ray_tracing/AccelerationStructure.h"



trc::rt::GeometryInstance::GeometryInstance(
    glm::mat3x4 transform,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(glm::transpose(transform)),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}

trc::rt::GeometryInstance::GeometryInstance(
    mat4 transform,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(glm::transpose(transform)),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}

trc::rt::GeometryInstance::GeometryInstance(
    glm::mat3x4 transform,
    ui32 instanceCustomIndex,
    ui8 mask,
    ui32 shaderBindingTableRecordOffset,
    vk::GeometryInstanceFlagsKHR flags,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(glm::transpose(transform)),
    instanceCustomIndex(instanceCustomIndex),
    mask(mask),
    shaderBindingTableRecordOffset(shaderBindingTableRecordOffset),
    flags(static_cast<ui32>(flags)),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}

trc::rt::GeometryInstance::GeometryInstance(
    mat4 transform,
    ui32 instanceCustomIndex,
    ui8 mask,
    ui32 shaderBindingTableRecordOffset,
    vk::GeometryInstanceFlagsKHR flags,
    const BottomLevelAccelerationStructure& blas)
    :
    transform(glm::transpose(transform)),
    instanceCustomIndex(instanceCustomIndex),
    mask(mask),
    shaderBindingTableRecordOffset(shaderBindingTableRecordOffset),
    flags(static_cast<ui32>(flags)),
    accelerationStructureAddress(blas.getDeviceAddress())
{
}

void trc::rt::GeometryInstance::setTransform(const mat4& t)
{
    transform = glm::transpose(t);
}
