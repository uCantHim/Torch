#pragma once

#include "AssetIds.h"
#include "Node.h"

namespace trc::rt
{
    auto makeGeometryInfo(GeometryID geo) -> vk::AccelerationStructureGeometryKHR;
    auto makeGeometryInfo(const Geometry& geo) -> vk::AccelerationStructureGeometryKHR;

    struct [[deprecated]] GeometryInstance
    {
        glm::mat3x4 transform;
        ui32 instanceCustomIndex : 24;
        ui32 mask : 8;
        ui32 shaderBindingTableRecordOffset : 24;
        ui32 flags : 8;  // vk::GeometryInstanceFlagsKHR
        ui64 accelerationStructureAddress;
    };

    /**
     * TODO: Something like this
     */
    class Instance : public Node
    {
    public:
        void update();

    private:
        glm::mat3x4 transform;
        ui32 instanceCustomIndex : 24;
        ui32 mask : 8;
        ui32 shaderBindingTableRecordOffset : 24;
        ui32 flags : 8;  // vk::GeometryInstanceFlagsKHR
        ui64 accelerationStructureAddress;
    };
}
