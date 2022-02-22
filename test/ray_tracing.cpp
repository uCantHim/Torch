/**
 * This file shows how Torch infrastructure could be used to implement very
 * basic ray tracing functionality.
 */

#include <iostream>

#include <trc/Torch.h>
#include <trc/DescriptorSetUtils.h>
#include <trc/TorchResources.h>
#include <trc/PipelineDefinitions.h>
#include <trc/ray_tracing/RayTracing.h>
using namespace trc::basic_types;

using trc::rt::BLAS;
using trc::rt::TLAS;

void run()
{
    auto torch = trc::initFull(
        trc::InstanceCreateInfo{ .enableRayTracing = true },
        trc::WindowCreateInfo{}
    );
    auto& device = torch->getDevice();
    auto& instance = torch->getInstance();
    auto& swapchain = torch->getWindow();
    auto& ar = torch->getAssetRegistry();

    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;
    camera.lookAt({ 0, 2, 3 }, { 0, 0, 0 }, { 0, 1, 0 });
    auto size = swapchain.getImageExtent();
    camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.1f, 100.0f);

    trc::Geometry geo = trc::loadGeometry(TRC_TEST_ASSET_DIR"/skeleton.fbx", ar).get().get();

    trc::Geometry tri = ar.add(
        trc::GeometryData{
            .vertices = {
                { vec3(0.0f, 0.0f, 0.0f), {}, {}, {} },
                { vec3(1.0f, 1.0f, 0.0f), {}, {}, {} },
                { vec3(0.0f, 1.0f, 0.0f), {}, {}, {} },
                { vec3(1.0f, 0.0f, 0.0f), {}, {}, {} },
            },
            .indices = {
                0, 1, 2,
                0, 3, 1,
            }
        }
    ).get();

    // --- BLAS --- //

    BLAS triBlas{ instance, tri };
    BLAS blas{ instance, geo };
    trc::rt::buildAccelerationStructures(instance, { &blas, &triBlas });


    // --- TLAS --- //

    std::vector<trc::rt::GeometryInstance> instances{
        // Skeleton
        {
            {
                0.03, 0, 0, -2,
                0, 0.03, 0, 0,
                0, 0, 0.03, 0
            },
            42,   // instance custom index
            0xff, // mask
            0,    // shader binding table offset
            vk::GeometryInstanceFlagBitsKHR::eForceOpaque
            | vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
            blas
        },
        // Triangle
        {
            {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0
            },
            43,   // instance custom index
            0xff, // mask
            0,    // shader binding table offset
            vk::GeometryInstanceFlagBitsKHR::eForceOpaque
            | vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
            triBlas
        }
    };
    vkb::Buffer instanceBuffer{
        instance.getDevice(),
        instances,
        vk::BufferUsageFlagBits::eShaderDeviceAddress
        | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal,
        vkb::DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    };

    TLAS tlas{ instance, 30 };
    tlas.build(*instanceBuffer, instances.size());


    // --- Descriptor sets --- //

    auto tlasDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .buildUnique(device);

    auto outputImageDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .buildUnique(device);

    std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
    };
    auto descPool = instance.getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        swapchain.getFrameCount() + 1,
        poolSizes
    ));

    vkb::FrameSpecific<vk::UniqueDescriptorSet> imageDescSets{
        swapchain,
        [&](ui32 imageIndex) -> vk::UniqueDescriptorSet
        {
            auto set = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
                { *descPool, 1, &*outputImageDescLayout }
            )[0]);

            vk::Image image = swapchain.getImage(imageIndex);
            vk::DescriptorImageInfo imageInfo(
                {}, // sampler
                swapchain.getImageView(imageIndex),
                vk::ImageLayout::eGeneral
            );
            vk::WriteDescriptorSet write(
                *set, 0, 0, 1,
                vk::DescriptorType::eStorageImage,
                &imageInfo
            );
            instance.getDevice()->updateDescriptorSets(write, {});

            return set;
        }
    };

    auto tlasDescSet = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
        { *descPool, 1, &*tlasDescLayout }
    )[0]);

    auto tlasHandle = *tlas;
    vk::StructureChain asWriteChain{
        vk::WriteDescriptorSet(
            *tlasDescSet, 0, 0, 1,
            vk::DescriptorType::eAccelerationStructureKHR,
            {}, {}, {}
        ),
        vk::WriteDescriptorSetAccelerationStructureKHR(tlasHandle)
    };
    instance.getDevice()->updateDescriptorSets(asWriteChain.get<vk::WriteDescriptorSet>(), {});


    // --- Ray Pipeline --- //

    constexpr ui32 maxRecursionDepth{ 16 };
    auto rayPipelineLayout = trc::makePipelineLayout(device,
        { *tlasDescLayout, *outputImageDescLayout },
        {
            // View and projection matrices
            { vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(mat4) * 2 },
        }
    );
    auto [rayPipeline, shaderBindingTable] = trc::rt::buildRayTracingPipeline(torch->getInstance())
        .addRaygenGroup(TRC_SHADER_DIR"/test/raygen.rgen.spv")
        .beginTableEntry()
            .addMissGroup(TRC_SHADER_DIR"/test/miss_blue.rmiss.spv")
            .addMissGroup(TRC_SHADER_DIR"/test/miss_orange.rmiss.spv")
        .endTableEntry()
        .addTrianglesHitGroup(
            TRC_SHADER_DIR"/test/closesthit.rchit.spv",
            TRC_SHADER_DIR"/test/anyhit.rahit.spv"
        )
        .addCallableGroup(TRC_SHADER_DIR"/test/callable.rcall.spv")
        .build(maxRecursionDepth, rayPipelineLayout);

    trc::DescriptorProvider tlasDescProvider{ *tlasDescLayout, *tlasDescSet };
    trc::FrameSpecificDescriptorProvider imageDescProvider{ *outputImageDescLayout, imageDescSets };
    rayPipeline.getLayout().addStaticDescriptorSet(0, tlasDescProvider);
    rayPipeline.getLayout().addStaticDescriptorSet(1, imageDescProvider);


    // --- Render Pass --- //

    trc::RayTracingPass rayPass;
    torch->getRenderConfig().getLayout().addPass(trc::rt::rayTracingRenderStage, rayPass);


    // --- Draw function --- //

    rayPass.addRayFunction(
        [
            &,
            &rayPipeline=rayPipeline,
            &shaderBindingTable=shaderBindingTable
        ](vk::CommandBuffer cmdBuf)
        {
            auto image = swapchain.getImage(swapchain.getCurrentFrame());

            // Bring image into general layout
            cmdBuf.pipelineBarrier(
                vk::PipelineStageFlagBits::eAllCommands,
                vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                vk::DependencyFlagBits::eByRegion,
                {}, {},
                vk::ImageMemoryBarrier(
                    {},
                    vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                )
            );

            rayPipeline.bind(cmdBuf);
            cmdBuf.pushConstants<mat4>(
                *rayPipeline.getLayout(), vk::ShaderStageFlagBits::eRaygenKHR,
                0, { camera.getViewMatrix(), camera.getProjectionMatrix() }
            );

            cmdBuf.traceRaysKHR(
                shaderBindingTable.getEntryAddress(0),
                shaderBindingTable.getEntryAddress(1),
                shaderBindingTable.getEntryAddress(2),
                shaderBindingTable.getEntryAddress(3),
                swapchain.getImageExtent().width,
                swapchain.getImageExtent().height,
                1,
                instance.getDL()
            );

            // Bring image into present layout
            cmdBuf.pipelineBarrier(
                vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                vk::PipelineStageFlagBits::eAllCommands,
                vk::DependencyFlagBits::eByRegion,
                {}, {},
                vk::ImageMemoryBarrier(
                    vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
                    {},
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::ePresentSrcKHR,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                )
            );
        }
    );

    while (swapchain.isOpen())
    {
        vkb::pollEvents();
        swapchain.drawFrame(torch->makeDrawConfig(*scene, camera));
    }

    instance.getDevice()->waitIdle();

    scene.reset();
}

int main()
{
    run();
    trc::terminate();

    return 0;
}
