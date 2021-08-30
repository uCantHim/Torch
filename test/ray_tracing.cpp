#include <iostream>

#include <trc/Torch.h>
#include <trc/DescriptorSetUtils.h>
#include <trc/TorchResources.h>
#include <trc/asset_import/AssetUtils.h>
#include <trc/ray_tracing/RayTracing.h>
using namespace trc::basic_types;

using trc::rt::BLAS;
using trc::rt::TLAS;

int main()
{
    auto torch = trc::initFull();
    auto& instance = *torch.instance;
    auto& swapchain = torch.window->getSwapchain();
    auto& ar = *torch.assetRegistry;

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

    vkb::MemoryPool asPool{ instance.getDevice(), 100000000 };
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
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    TLAS tlas{ instance, 30 };
    tlas.build(*instanceBuffer);


    // --- Ray Pipeline --- //

    auto rayDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .buildUnique(torch.instance->getDevice());

    auto outputImageDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .buildUnique(torch.instance->getDevice());

    auto layout = trc::makePipelineLayout(
        torch.instance->getDevice(),
        { *rayDescLayout, *outputImageDescLayout },
        {
            // View and projection matrices
            { vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(mat4) * 2 },
        }
    );

    constexpr ui32 maxRecursionDepth{ 16 };
    auto [rayPipeline, shaderBindingTable] =
        trc::rt::_buildRayTracingPipeline(torch.instance->getDevice())
        .addRaygenGroup(TRC_SHADER_DIR"/shaders/ray_tracing/raygen.rgen.spv")
        .beginTableEntry()
            .addMissGroup(TRC_SHADER_DIR"/shaders/ray_tracing/miss.rmiss.spv")
            .addMissGroup(TRC_SHADER_DIR"/shaders/ray_tracing/miss.rmiss.spv")
        .endTableEntry()
        .addTrianglesHitGroup(
            TRC_SHADER_DIR"/shaders/ray_tracing/closesthit.rchit.spv",
            TRC_SHADER_DIR"/shaders/ray_tracing/anyhit.rahit.spv"
        )
        .addCallableGroup(TRC_SHADER_DIR"/shaders/ray_tracing/callable.rcall.spv")
        .build(maxRecursionDepth, *layout);


    // --- Descriptor sets --- //

    std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
    };
    auto descPool = instance.getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        swapchain.getFrameCount() + 1,
        poolSizes
    ));

    std::vector<vk::UniqueImageView> imageViews;
    vkb::FrameSpecificObject<vk::UniqueDescriptorSet> imageDescSets{
        swapchain,
        [&](ui32 imageIndex) -> vk::UniqueDescriptorSet
        {
            auto set = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
                { *descPool, 1, &*outputImageDescLayout }
            )[0]);

            vk::Image image = swapchain.getImage(imageIndex);
            vk::ImageView imageView = *imageViews.emplace_back(
                instance.getDevice()->createImageViewUnique(
                    vk::ImageViewCreateInfo(
                        {},
                        image,
                        vk::ImageViewType::e2D,
                        swapchain.getImageFormat(),
                        {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                    )
                )
            );
            vk::DescriptorImageInfo imageInfo(
                {}, // sampler
                *imageViews.emplace_back(swapchain.createImageView(imageIndex)),
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

    auto asDescSet = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
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
    instance.getDevice()->updateDescriptorSets(asWriteChain.get<vk::WriteDescriptorSet>(), {});


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
    RayTracingRenderPass rayPass;

    torch.renderConfig->getGraph().after(trc::RenderStageTypes::getDeferred(), rayStageTypeId);
    torch.renderConfig->getGraph().addPass(rayStageTypeId, rayPass);

    scene->registerDrawFunction(
        rayStageTypeId, trc::SubPass::ID(0), trc::internal::getFinalLightingPipeline(),
        [
            &,
            pipeline=*rayPipeline,
            &shaderBindingTable=shaderBindingTable
        ](const trc::DrawEnvironment&, vk::CommandBuffer cmdBuf)
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

        torch.window->drawFrame(trc::DrawConfig{
            .scene=scene.get(),
            .camera=&camera,
            .renderConfig=torch.renderConfig.get(),
        });
    }

    instance.getDevice()->waitIdle();

    scene.reset();
    torch.renderConfig.reset();
    torch.window.reset();
    torch.assetRegistry.reset();
    torch.instance.reset();
    trc::terminate();

    return 0;
}
