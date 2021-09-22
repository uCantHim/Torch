#include <iostream>

#include <vkb/ImageUtils.h>

#include <trc/Torch.h>
#include <trc/DescriptorSetUtils.h>
#include <trc/TorchResources.h>
#include <trc/asset_import/AssetUtils.h>
#include <trc/ray_tracing/RayTracing.h>
#include <trc/ray_tracing/FinalCompositingPass.h>
#include <trc/ray_tracing/RayScene.h>
using namespace trc::basic_types;

using trc::rt::BLAS;
using trc::rt::TLAS;

int main()
{
    auto torch = trc::initFull(
        trc::InstanceCreateInfo{ .enableRayTracing = true },
        trc::WindowCreateInfo{
            .swapchainCreateInfo={ .imageUsage=vk::ImageUsageFlagBits::eTransferDst }
        }
    );
    auto& instance = *torch.instance;
    auto& device = torch.instance->getDevice();
    auto& swapchain = torch.window->getSwapchain();
    auto& ar = *torch.assetRegistry;

    auto scene = std::make_unique<trc::Scene>();
    trc::Camera camera;
    camera.lookAt({ 0, 2, 4 }, { 0, 0, 0 }, { 0, 1, 0 });
    auto size = swapchain.getImageExtent();
    camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.1f, 100.0f);

    // Create some rasterized stuff
    trc::Drawable sphere(
        trc::loadGeometry(TRC_TEST_ASSET_DIR"/sphere.fbx", ar).get(),
        ar.add(trc::Material{
            .color=vec4(0.3f, 0.3f, 0.3f, 1),
            .kSpecular=vec4(0.3f, 0.3f, 0.3f, 1),
            .reflectivity=1.0f,
        })
    );
    sphere.attachToScene(*scene);
    sphere.translate(-1.5f, 0.5f, 1.0f).setScale(0.2f);
    trc::Node sphereNode;
    sphereNode.attach(sphere);
    scene->getRoot().attach(sphereNode);

    trc::Drawable plane(
        ar.add(trc::makePlaneGeo()),
        ar.add(trc::Material{ .reflectivity=1.0f }),
        *scene
    );
    plane.rotateX(glm::radians(90.0f))
        .rotateY(glm::radians(-15.0f))
        .translate(0.5f, 0.5f, -1.0f)
        .setScale(3.0f, 1.0f, 1.7f);

    trc::GeometryID treeGeo = trc::loadGeometry(TRC_TEST_ASSET_DIR"/tree_lowpoly.fbx", ar).get();
    trc::MaterialID treeMat = ar.add(trc::Material{ .color=vec4(0, 1, 0, 1) });
    trc::Drawable tree(treeGeo, treeMat, *scene);
    tree.rotateX(-glm::half_pi<float>()).setScale(0.1f);

    trc::Drawable floor(
        ar.add(trc::makePlaneGeo(50.0f, 50.0f, 60, 60)),
        ar.add(trc::Material{
            .kSpecular=vec4(0.2f),
            .reflectivity=0.3f,
            .diffuseTexture=ar.add(
                vkb::loadImage2D(device, TRC_TEST_ASSET_DIR"/tex_pavement_grassy_albedo.tif")
            ),
            .bumpTexture=ar.add(
                vkb::loadImage2D(device, TRC_TEST_ASSET_DIR"/tex_pavement_grassy_normal.tif")
            ),
        })
    );
    floor.attachToScene(*scene);

    auto sun = scene->getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.5f);
    //auto& shadow = scene->enableShadow(sun, { .shadowMapResolution=uvec2(2048, 2048) }, *torch.shadowPool);
    //shadow.setProjectionMatrix(glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 30.0f));
    //scene->getRoot().attach(shadow);


    // --- Ray tracing scene --- //

    trc::rt::RayScene rayScene(instance);
    rayScene.addDrawable({ floor.getGlobalTransformID(), floor.getGeometry(), floor.getMaterial() });
    rayScene.addDrawable({ sphere.getGlobalTransformID(), sphere.getGeometry(), sphere.getMaterial() });
    rayScene.addDrawable({ plane.getGlobalTransformID(), plane.getGeometry(), plane.getMaterial() });
    rayScene.addDrawable({ tree.getGlobalTransformID(), treeGeo, treeMat });


    // --- Descriptor sets --- //

    auto tlasDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, trc::rt::ALL_RAY_PIPELINE_STAGE_FLAGS)
        .buildUnique(torch.instance->getDevice());

    std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1),
    };
    auto descPool = instance.getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        1,
        poolSizes
    ));

    auto tlasDescSet = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
        { *descPool, 1, &*tlasDescLayout }
    )[0]);

    auto tlasHandle = *rayScene.getTlas();
    vk::StructureChain asWriteChain{
        vk::WriteDescriptorSet(
            *tlasDescSet, 0, 0, 1,
            vk::DescriptorType::eAccelerationStructureKHR,
            {}, {}, {}
        ),
        vk::WriteDescriptorSetAccelerationStructureKHR(tlasHandle)
    };
    vk::DescriptorBufferInfo bufferInfo(rayScene.getDrawableDataBuffer(), 0, VK_WHOLE_SIZE);
    vk::WriteDescriptorSet bufferWrite(
        *tlasDescSet, 1, 0, vk::DescriptorType::eStorageBuffer,
        {}, bufferInfo
    );
    instance.getDevice()->updateDescriptorSets(
        { asWriteChain.get<vk::WriteDescriptorSet>(), bufferWrite },
        {}
    );


    // --- Output Images --- //

    vkb::FrameSpecific<trc::rt::RayBuffer> rayBuffer{
        swapchain,
        [&](ui32) {
            return trc::rt::RayBuffer(
                device,
                { swapchain.getSize(), vk::ImageUsageFlagBits::eTransferSrc }
            );
        }
    };


    // --- Render Pass --- //

    auto rayStageTypeId = trc::RenderStageType::createAtNextIndex(1).first;
    trc::RayTracingPass rayPass;

    torch.renderConfig->getGraph().after(trc::RenderStageTypes::getDeferred(), rayStageTypeId);
    torch.renderConfig->getGraph().addPass(rayStageTypeId, rayPass);


    // Add the final compositing pass that merges rasterization and ray tracing results
    trc::rt::FinalCompositingPass compositing(
        *torch.window,
        {
            .gBuffer = &torch.renderConfig->getGBuffer(),
            .rayBuffer = &rayBuffer,
            .assetRegistry = &ar,
        }
    );

    auto& graph = torch.renderConfig->getGraph();
    graph.after(rayStageTypeId, trc::rt::getFinalCompositingStage());
    graph.addPass(trc::rt::getFinalCompositingStage(), compositing);


    // --- Ray Pipeline --- //

    constexpr ui32 maxRecursionDepth{ 16 };
    auto [rayPipeline, shaderBindingTable] =
        trc::rt::buildRayTracingPipeline(*torch.instance)
        .addRaygenGroup(TRC_SHADER_DIR"/ray_tracing/reflect.rgen.spv")
        .beginTableEntry()
            .addMissGroup(TRC_SHADER_DIR"/ray_tracing/blue.rmiss.spv")
        .endTableEntry()
        .addTrianglesHitGroup(
            TRC_SHADER_DIR"/ray_tracing/reflect.rchit.spv",
            TRC_SHADER_DIR"/ray_tracing/anyhit.rahit.spv"
        )
        .build(
            maxRecursionDepth,
            trc::makePipelineLayout(torch.instance->getDevice(),
                {
                    *tlasDescLayout,
                    compositing.getInputImageDescriptor().getDescriptorSetLayout(),
                    torch.assetRegistry->getDescriptorSetProvider().getDescriptorSetLayout(),
                    torch.renderConfig->getSceneDescriptorProvider().getDescriptorSetLayout(),
                    torch.shadowPool->getProvider().getDescriptorSetLayout(),
                },
                {
                    // View and projection matrices
                    { vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(mat4) * 2 },
                }
            )
        );

    trc::DescriptorProvider tlasDescProvider{ *tlasDescLayout, *tlasDescSet };
    trc::FrameSpecificDescriptorProvider reflectionImageProvider(
        rayBuffer->getImageDescriptorLayout(),
        {
            swapchain,
            [&](ui32 i) {
                return rayBuffer.getAt(i).getImageDescriptor(trc::rt::RayBuffer::eReflections).getDescriptorSet();
            }
        }
    );
    rayPipeline.addStaticDescriptorSet(0, tlasDescProvider);
    rayPipeline.addStaticDescriptorSet(1, compositing.getInputImageDescriptor());
    rayPipeline.addStaticDescriptorSet(2, torch.assetRegistry->getDescriptorSetProvider());
    rayPipeline.addStaticDescriptorSet(3, torch.renderConfig->getSceneDescriptorProvider());
    rayPipeline.addStaticDescriptorSet(4, torch.shadowPool->getProvider());


    // --- Draw function --- //

    scene->registerDrawFunction(
        rayStageTypeId, trc::SubPass::ID(0), trc::internal::getFinalLightingPipeline(),
        [
            &,
            &rayPipeline=rayPipeline,
            &shaderBindingTable=shaderBindingTable
        ](const trc::DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            vk::Image image = *rayBuffer->getImage(trc::rt::RayBuffer::eReflections);

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
            rayPipeline.bindStaticDescriptorSets(cmdBuf);
            cmdBuf.pushConstants<mat4>(
                rayPipeline.getLayout(), vk::ShaderStageFlagBits::eRaygenKHR,
                0, { camera.getViewMatrix(), camera.getProjectionMatrix() }
            );

            cmdBuf.traceRaysKHR(
                shaderBindingTable.getEntryAddress(0),
                shaderBindingTable.getEntryAddress(1),
                shaderBindingTable.getEntryAddress(2),
                {},
                swapchain.getImageExtent().width,
                swapchain.getImageExtent().height,
                1,
                instance.getDL()
            );
        }
    );


    vkb::on<vkb::KeyPressEvent>([&](auto& e) {
        static bool count{ false };
        if (e.key == vkb::Key::r)
        {
            auto& mat = ar.get(floor.getMaterial());
            mat.reflectivity = 0.3f * float(count);
            ar.updateMaterials();
            count = !count;
        }
    });

    vkb::Timer timer;
    while (swapchain.isOpen())
    {
        vkb::pollEvents();

        sphereNode.setRotation(timer.duration() / 1000.0f * 0.5f, vec3(0, 1, 0));

        scene->updateTransforms();
        device.executeCommandsSynchronously(
            vkb::QueueType::compute,
            [&](vk::CommandBuffer cmdBuf) {
                rayScene.update(cmdBuf);
            }
        );

        torch.window->drawFrame(trc::DrawConfig{
            .scene=scene.get(),
            .camera=&camera,
            .renderConfig=torch.renderConfig.get(),
            .renderArea={ torch.window->makeFullscreenRenderArea() }
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
