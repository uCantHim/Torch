#include "trc/drawable/RayComponent.h"



trc::RayComponent::RayComponent(const RayComponentCreateInfo& info)
    :
    modelMatrix(info.transformation),
    geo(info.geo.getDeviceDataHandle()),
    materialIndex(0),
    instanceDataIndex(0) // Set by ComponentTraits::onCreate
{
    if (!geo.hasAccelerationStructure())
    {
        log::error << log::here() << ": Tried to create a ray tracing component, but the"
                   << " associated geometry " << info.geo.getMetadata().name
                   << " does not have an acceleration structure."
                   << " The ray tracing component will not be created.";

        throw std::invalid_argument("Unable to create ray component for geometry without an"
                                    " acceleration structure.");
    }
}

void componentlib::ComponentTraits<trc::RayComponent>::onCreate(
    trc::DrawableComponentScene& storage,
    trc::DrawableID /*drawable*/,
    trc::RayComponent& ray)
{
    // Allocate a user data structure
    //
    // This data is referenced by geometry instances via the instanceCustomIndex
    // property and defines auxiliary information that Torch needs to draw
    // ray traced objects.
    ray.instanceDataIndex = storage.getRayModule().allocateRayInstance(
        trc::RaySceneModule::RayInstanceData{
            .geometryIndex=ray.geo.getDeviceIndex(),
            .materialIndex=ray.materialIndex,
        },
        0xff, 0,
        vk::GeometryInstanceFlagBitsKHR::eForceOpaque
        | vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
        ray.geo.getAccelerationStructure()
    );
}

void componentlib::ComponentTraits<trc::RayComponent>::onDelete(
    trc::DrawableComponentScene& storage,
    trc::DrawableID /*id*/,
    trc::RayComponent ray)
{
    storage.getRayModule().freeRayInstance(ray.instanceDataIndex);
}
