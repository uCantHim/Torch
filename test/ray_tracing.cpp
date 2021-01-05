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
    camera.lookAt({ 0, 2, 3 }, { 0, 0, 0 }, { 0, 1, 0 });
    auto size = vkb::getSwapchain().getImageExtent();
    camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.1f, 100.0f);

    trc::GeometryID geoId = trc::loadGeometry("assets/skeleton.fbx")
                            >> trc::AssetRegistry::addGeometry;

    trc::GeometryID triId = trc::AssetRegistry::addGeometry(
        trc::Geometry(
            trc::MeshData{
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
        )
    );

    // --- BLAS --- //

    BLAS blas{ geoId };
    blas.build();

    BLAS triBlas{ triId };
    triBlas.build();


    // --- TLAS --- //

    std::vector<trc::rt::GeometryInstance> instances{
        // Skeleton
        //{
        //    {
        //        1, 0, 0, 0,
        //        0, 1, 0, 0,
        //        0, 0, 1, 0
        //    },
        //    42,   // instance custom index
        //    0xff, // mask
        //    0,    // shader binding table offset
        //    static_cast<ui32>(
        //        vk::GeometryInstanceFlagBitsKHR::eForceOpaque
        //        | vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable
        //    ),
        //    blas.getDeviceAddress()
        //},
        // Triangle
        {
            {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0
            },
            42,   // instance custom index
            0xff, // mask
            0,    // shader binding table offset
            static_cast<ui32>(
                vk::GeometryInstanceFlagBitsKHR::eForceOpaque
                | vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable
            ),
            triBlas.getDeviceAddress()
        }
    };
    vkb::Buffer instanceBuffer{
        instances,
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
        { *rayDescLayout, *outputImageDescLayout },
        {
            // View and projection matrices
            { vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(mat4) * 2 },
        }
    );

    auto [rayPipeline, shaderBindingTable] = trc::rt::_buildRayTracingPipeline()
        .addRaygenGroup("shaders/ray_tracing/raygen.rgen.spv")
        .addMissGroup("shaders/ray_tracing/miss.rmiss.spv")
        .addTrianglesHitGroup(
            "shaders/ray_tracing/closesthit.rchit.spv",
            "shaders/ray_tracing/anyhit.rahit.spv"
        )
        .addCallableGroup("shaders/ray_tracing/callable.rcall.spv")
        .build(16, *layout);


    // --- Descriptor sets --- //

    auto descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        vkb::getSwapchain().getFrameCount() + 1,
        std::vector<vk::DescriptorPoolSize>{
            vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
        }
    ));

    std::vector<vk::UniqueImageView> imageViews;
    vkb::FrameSpecificObject<vk::UniqueDescriptorSet> imageDescSets{
        [&](ui32 imageIndex) -> vk::UniqueDescriptorSet
        {
            auto set = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
                { *descPool, 1, &*outputImageDescLayout }
            )[0]);

            vk::Image image = vkb::getSwapchain().getImage(imageIndex);
            vk::ImageView imageView = *imageViews.emplace_back(
                vkb::getDevice()->createImageViewUnique(
                    vk::ImageViewCreateInfo(
                        {},
                        image,
                        vk::ImageViewType::e2D,
                        vkb::getSwapchain().getImageFormat(),
                        {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                    )
                )
            );
            vk::DescriptorImageInfo imageInfo(
                {}, // sampler
                *imageViews.emplace_back(vkb::getSwapchain().createImageView(imageIndex)),
                vk::ImageLayout::eGeneral
            );
            vk::WriteDescriptorSet write(
                *set, 0, 0, 1,
                vk::DescriptorType::eStorageImage,
                &imageInfo
            );
            vkb::getDevice()->updateDescriptorSets(write, {});

            return set;
        }
    };

    auto asDescSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
        { *descPool, 1, &*rayDescLayout }
    )[0]);

    auto tlasHandle = *tlas;
    vk::StructureChain<
        vk::WriteDescriptorSet,
        vk::WriteDescriptorSetAccelerationStructureKHR
    > asWriteChain{
        vk::WriteDescriptorSet(
            *asDescSet, 0, 0, 1,
            vk::DescriptorType::eAccelerationStructureKHR,
            {}, {}, {}
        ),
        vk::WriteDescriptorSetAccelerationStructureKHR(tlasHandle)
    };
    vkb::getDevice()->updateDescriptorSets(asWriteChain.get<vk::WriteDescriptorSet>(), {});


    class RayTracingRenderPass : public trc::RenderPass
    {
    public:
        RayTracingRenderPass()
            : RenderPass({}, 1)
        {}

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override {}
        void end(vk::CommandBuffer cmdBuf) override {}
    };

    auto rayStageTypeId = trc::RenderStageType::createAtNextIndex(1).first;
    auto rayPassId = trc::RenderPass::createAtNextIndex<RayTracingRenderPass>().first;
    //auto rayPipelineId = trc::Pipeline::createAtNextIndex(
    //    std::move(layout), std::move(rayPipeline),
    //    vk::PipelineBindPoint::eRayTracingKHR
    //).first;

    renderer->getRenderGraph().after(trc::RenderStageTypes::getDeferred(), rayStageTypeId);
    renderer->getRenderGraph().addPass(rayStageTypeId, rayPassId);

    scene->registerDrawFunction(
        rayStageTypeId, 0, trc::internal::Pipelines::eFinalLighting,
        [
            &,
            pipeline=*rayPipeline,
            &shaderBindingTable=shaderBindingTable
        ](const trc::DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            auto image = vkb::getSwapchain().getImage(vkb::getSwapchain().getCurrentFrame());

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

            cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline);
            cmdBuf.pushConstants<mat4>(
                *layout, vk::ShaderStageFlagBits::eRaygenKHR,
                0, { camera.getViewMatrix(), camera.getProjectionMatrix() }
            );
            cmdBuf.bindDescriptorSets(
                vk::PipelineBindPoint::eRayTracingKHR, *layout,
                0, { *asDescSet, **imageDescSets },
                {} // Dynamic offsets
            );

            cmdBuf.traceRaysKHR(
                shaderBindingTable.getShaderGroupAddress(0),
                shaderBindingTable.getShaderGroupAddress(1),
                shaderBindingTable.getShaderGroupAddress(2),
                shaderBindingTable.getShaderGroupAddress(3),
                vkb::getSwapchain().getImageExtent().width,
                vkb::getSwapchain().getImageExtent().height,
                1,
                vkb::getDL()
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
