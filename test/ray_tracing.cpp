#include <iostream>

#include <trc/Torch.h>
#include <trc/DescriptorSetUtils.h>
#include <trc/AssetUtils.h>
#include <trc/ray_tracing/RayTracing.h>
using namespace trc::basic_types;

using trc::rt::BLAS;
using trc::rt::TLAS;

int main()
{
    auto renderer = trc::init({ .enableRayTracing=true });
    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;

    trc::GeometryID geoId = trc::loadGeometry("assets/skeleton.fbx")
                            >> trc::AssetRegistry::addGeometry;

    // --- BLAS --- //

    BLAS blas{ geoId };
    blas.build();


    // --- TLAS --- //

    trc::rt::GeometryInstance blasInstance{
        {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0
        },
        42,   // instance custom index
        0xff, // mask
        0,    // shader binding table offset
        static_cast<ui32>(vk::GeometryInstanceFlagBitsKHR::eForceOpaque),
        blas.getDeviceAddress()
    };
    vkb::Buffer instanceBuffer{
        sizeof(vk::AccelerationStructureInstanceKHR),
        &blasInstance,
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    TLAS tlas{ 30 };
    tlas.build(instanceBuffer);


    // --- Ray Pipeline --- //

    auto rayDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .buildUnique(vkb::getDevice());

    auto outputImageDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .buildUnique(vkb::getDevice());

    auto layout = trc::makePipelineLayout(
        { *rayDescLayout, *outputImageDescLayout }, // Descriptor set layouts
        {}  // Push constant ranges
    );
    auto rayPipeline = trc::rt::buildRayTracingPipeline()
        .addGroup<trc::rt::RayShaderGroup::eGeneral>()
            .addStage<trc::rt::RayShaderStage::eRaygen>("shaders/ray_tracing/raygen.rgen.spv")
        .build(16, *layout);



    while (vkb::getSwapchain().isOpen())
    {
        vkb::pollEvents();
        renderer->drawFrame(*scene, camera);
    }

    vkb::getDevice()->waitIdle();
    scene.reset();
    renderer.reset();
    trc::terminate();
    return 0;
}
