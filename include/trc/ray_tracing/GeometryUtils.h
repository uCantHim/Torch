#pragma once

#include "AssetIds.h"
#include "Node.h"

namespace trc::rt
{
    class BottomLevelAccelerationStructure;

    auto makeGeometryInfo(const Geometry& geo) -> vk::AccelerationStructureGeometryKHR;

    struct GeometryInstance
    {
        GeometryInstance() = default;

        GeometryInstance(glm::mat3x4 transform,
                         const BottomLevelAccelerationStructure& blas);
        GeometryInstance(mat4 transform,
                         const BottomLevelAccelerationStructure& blas);

        GeometryInstance(glm::mat3x4 transform,
                         ui32 instanceCustomIndex,
                         ui8 mask,
                         ui32 shaderBindingTableRecordOffset,
                         vk::GeometryInstanceFlagsKHR flags,
                         const BottomLevelAccelerationStructure& blas);

        GeometryInstance(mat4 transform,
                         ui32 instanceCustomIndex,
                         ui8 mask,
                         ui32 shaderBindingTableRecordOffset,
                         vk::GeometryInstanceFlagsKHR flags,
                         const BottomLevelAccelerationStructure& blas);

        glm::mat3x4 transform;
        ui32 instanceCustomIndex : 24 { 0 };
        ui8 mask : 8 { 0xff };
        ui32 shaderBindingTableRecordOffset : 24 { 0 };
        ui8 flags : 8 { 0 };  // vk::GeometryInstanceFlagsKHR
        ui64 accelerationStructureAddress;
    };

    class Instance : public Node
    {
    public:
        /**
         * Updates the instance's transform
         */
        auto getInstanceData() const -> const GeometryInstance&;

    private:
        GeometryInstance instanceData;
    };
}
