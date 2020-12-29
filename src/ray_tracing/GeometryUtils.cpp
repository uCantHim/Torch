#include "ray_tracing/GeometryUtils.h"

#include "AssetRegistry.h"



auto trc::rt::makeGeometryInfo(GeometryID geoId) -> vk::AccelerationStructureGeometryKHR
{
    return makeGeometryInfo(**AssetRegistry::getGeometry(geoId));
}

auto trc::rt::makeGeometryInfo(const Geometry& geo) -> vk::AccelerationStructureGeometryKHR
{
    return { // Array of geometries in the AS
        vk::GeometryTypeKHR::eTriangles,
        vk::AccelerationStructureGeometryDataKHR{ // a union
            vk::AccelerationStructureGeometryTrianglesDataKHR{
                vk::Format::eR32G32B32Sfloat,
                vkb::getDevice()->getBufferAddress({ geo.getVertexBuffer() }),
                sizeof(trc::Vertex),
                geo.getIndexCount(), // max vertex
                vk::IndexType::eUint32,
                vkb::getDevice()->getBufferAddress({ geo.getIndexBuffer() }),
                nullptr // transform data
            }
        }
    };
}
