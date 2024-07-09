#include <trc/DescriptorSetUtils.h>
#include <trc/Torch.h>
#include <trc/base/Barriers.h>
#include <trc/core/Instance.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/core/Window.h>
#include <trc/drawable/DrawableScene.h>
#include <trc/ray_tracing/RayPipelineBuilder.h>
#include <trc/ray_tracing/ShaderBindingTable.h>

using namespace trc::basic_types;

int main()
{
    const trc::RenderStage kRayStage = trc::RenderStage::make();

    trc::init();
    trc::Instance instance({ .enableRayTracing=true, .deviceExtensions={}, .deviceFeatures={} });
    trc::Window window(instance);
    auto target = trc::makeRenderTarget(window);
    auto& device = instance.getDevice();

    // Create a triangle geometry
    trc::GeometryData geo{
        .vertices={
            trc::MeshVertex{ { -0.5f, -0.5f, 0.0f }, {}, {}, {} },
            trc::MeshVertex{ { 0.5f,  -0.5f, 0.0f }, {}, {}, {} },
            trc::MeshVertex{ { 0.0f,  0.5f,  0.0f }, {}, {}, {} },
        },
        .skeletalVertices={},
        .indices={ 0, 1, 2 },
    };
    trc::DeviceLocalBuffer vertBuf(
        device, geo.vertices,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        trc::DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    );
    trc::DeviceLocalBuffer indexBuf(
        device, geo.indices,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        trc::DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    );

    // Botton-level acceleration structure
    trc::rt::BottomLevelAccelerationStructure blas(
        instance,
        vk::AccelerationStructureGeometryKHR{ // Array of geometries in the AS
            vk::GeometryTypeKHR::eTriangles,
            vk::AccelerationStructureGeometryDataKHR{ // a union
                vk::AccelerationStructureGeometryTrianglesDataKHR{
                    vk::Format::eR32G32B32Sfloat,
                    device->getBufferAddress({ *vertBuf }),
                    sizeof(trc::MeshVertex),
                    static_cast<ui32>(geo.indices.size()),
                    vk::IndexType::eUint32,
                    device->getBufferAddress({ *indexBuf }),
                    nullptr // transform data
                }
            }
        }
    );
    blas.build();

    // Top-level acceleration structure
    vk::AccelerationStructureInstanceKHR triInstance(
        vk::TransformMatrixKHR({{
            {{ 1, 0, 0, 0 }},
            {{ 0, 1, 0, 0 }},
            {{ 0, 0, 1, 0 }},
        }}),
        0, 0xff, 0,
        vk::GeometryInstanceFlagBitsKHR::eForceOpaque
            | vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
        device->getAccelerationStructureAddressKHR({ *blas }, instance.getDL())
    );
    trc::DeviceLocalBuffer instancesBuffer{
        device, sizeof(vk::AccelerationStructureInstanceKHR), &triInstance,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        trc::DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    };

    trc::rt::TopLevelAccelerationStructure tlas(instance, 1);
    tlas.build(*instancesBuffer, 1);

    // Create the descriptor set
    //
    // The ray generation shader will need:
    //  - The top-level acceleration structure
    //  - The swapchain image in which to store the calculated color
    const ui32 imageCount = window.getFrameCount();
    auto descLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1, vk::ShaderStageFlagBits::eRaygenKHR)
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR)
        .build(instance.getDevice());

    std::vector<vk::DescriptorPoolSize> sizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, imageCount),
    };
    auto pool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        imageCount, sizes
    ));

    std::vector<vk::DescriptorSetLayout> descLayouts{ *descLayout, *descLayout, *descLayout };
    auto descSets = device->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(*pool, descLayouts));

    for (ui32 i = 0; auto& set : descSets)
    {
        vk::StructureChain tlasDescWrite{
            vk::WriteDescriptorSet(*set, 0, 0, 1, vk::DescriptorType::eAccelerationStructureKHR,
                                   {}, {}, {}),
            vk::WriteDescriptorSetAccelerationStructureKHR(*tlas)
        };
        vk::DescriptorImageInfo imageInfo({}, target.getImageView(i), vk::ImageLayout::eGeneral);
        device->updateDescriptorSets(
            {
                tlasDescWrite.get(),
                vk::WriteDescriptorSet(*set, 1, 0, 1, vk::DescriptorType::eStorageImage, &imageInfo)
            },
            {}
        );
        ++i;
    }

    // Pipeline
    trc::RenderPipelineBuilder renderConfig{ instance };
    renderConfig.getRenderGraph().insert(kRayStage);

    auto layout = trc::buildPipelineLayout()
        .addPushConstantRange({ vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(mat4) * 2 })
        .addStaticDescriptor(
            *descLayout,
            std::make_shared<trc::FrameSpecificDescriptorProvider>(
                trc::FrameSpecific<vk::DescriptorSet>(window, [&](ui32 i){ return *descSets[i]; })
            )
        )
        .build(instance.getDevice(), renderConfig.getResourceConfig());

    auto [pipeline, shaderBindingTable] = trc::rt::buildRayTracingPipeline(instance)
        .addRaygenGroup(trc::ShaderPath("basic_ray/raygen.rgen"))
        .addMissGroup(trc::ShaderPath("basic_ray/miss.rmiss"))
        .addTrianglesHitGroup(trc::ShaderPath("basic_ray/closesthit.rchit"),
                              trc::ShaderPath("basic_ray/anyhit.rahit"))
        .build(16, layout);
    auto& sbt = shaderBindingTable;

    // Other stuff
    trc::DrawableScene scene;
    trc::Camera camera;
    camera.lookAt(vec3(0, 0, -2), vec3(0, 0, 0), vec3(0, 1, 0));

    trc::Renderer renderer{ device, window };
    auto viewportConfig = renderConfig.makeViewport(device, trc::Viewport{});

    while (!window.isPressed(trc::Key::escape))
    {
        trc::pollEvents();

        auto frame = std::make_unique<trc::Frame>();
        auto& viewport = frame->addViewport(*viewportConfig, scene);

        viewport.taskQueue.spawnTask(
            kRayStage,
            trc::makeTask([&, &pipeline=pipeline](vk::CommandBuffer cmdBuf, trc::TaskEnvironment&)
            {
                trc::imageMemoryBarrier(
                    cmdBuf,
                    target.getCurrentImage(),
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                    vk::PipelineStageFlagBits::eAllCommands,
                    vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                    vk::AccessFlagBits::eHostWrite,
                    vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                );
                pipeline.bind(cmdBuf);
                cmdBuf.pushConstants<mat4>(*pipeline.getLayout(), vk::ShaderStageFlagBits::eRaygenKHR,
                                           0, { camera.getViewMatrix(), camera.getProjectionMatrix() });
                cmdBuf.traceRaysKHR(
                    sbt.getEntryAddress(0),
                    sbt.getEntryAddress(1),
                    sbt.getEntryAddress(2), // hit
                    {}, // callable
                    target.getSize().x, target.getSize().y, 1,
                    instance.getDL()
                );
                trc::imageMemoryBarrier(
                    cmdBuf,
                    target.getCurrentImage(),
                    vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
                    vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                    vk::PipelineStageFlagBits::eHost,
                    vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
                    vk::AccessFlagBits::eHostRead,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                );
            })
        );

        renderer.renderFrameAndPresent(std::move(frame), window);
    }

    renderer.waitForAllFrames();
    trc::terminate();

    return 0;
}
