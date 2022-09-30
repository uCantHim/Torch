#include <future>
#include <iostream>

#include <vkb/ImageUtils.h>
#include <trc_util/Timer.h>
#include <trc/DescriptorSetUtils.h>
#include <trc/PipelineDefinitions.h>
#include <trc/Torch.h>
#include <trc/TorchResources.h>
#include <trc/ray_tracing/RayTracing.h>
using namespace trc::basic_types;

using trc::rt::BLAS;
using trc::rt::TLAS;

void run()
{
    auto torch = trc::initFull(
        trc::InstanceCreateInfo{ .enableRayTracing = true },
        trc::WindowCreateInfo{
            .swapchainCreateInfo={ .imageUsage=vk::ImageUsageFlagBits::eTransferDst }
        }
    );
    auto& instance = torch->getInstance();
    auto& device = instance.getDevice();
    auto& window = torch->getWindow();
    auto& assets = torch->getAssetManager();

    auto scene = std::make_unique<trc::Scene>();

    // Camera
    trc::Camera camera;
    camera.lookAt({ 0, 2, 4 }, { 0, 0, 0 }, { 0, 1, 0 });
    auto size = window.getImageExtent();
    camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.1f, 100.0f);

    // Create some objects
    trc::Drawable sphere(
        trc::DrawableCreateInfo{
            assets.create(trc::makeSphereGeo()),
            assets.create(trc::MaterialData{
                .color=vec4(0.8f, 0.3f, 0.6f, 1),
                .specularKoefficient=vec4(0.3f, 0.3f, 0.3f, 1),
                .reflectivity=1.0f,
            }),
            false, true, true, true
        },
        *scene
    );
    sphere.translate(-1.5f, 0.5f, 1.0f).setScale(0.2f);
    trc::Node sphereNode;
    sphereNode.attach(sphere);
    scene->getRoot().attach(sphereNode);

    trc::Drawable plane({
        assets.create(trc::makePlaneGeo()),
        assets.create(trc::MaterialData{ .reflectivity=0.9f }),
        false, true, true, true
    }, *scene);
    plane.rotate(glm::radians(90.0f), glm::radians(-15.0f), 0.0f)
         .translate(0.5f, 0.5f, -1.0f)
         .setScale(3.0f, 1.0f, 1.7f);

    trc::GeometryID treeGeo = assets.create(trc::loadGeometry(TRC_TEST_ASSET_DIR"/tree_lowpoly.fbx"));
    trc::MaterialID treeMat = assets.create(trc::MaterialData{ .color=vec4(0, 1, 0, 1) });
    trc::Drawable tree({ treeGeo, treeMat, false, true, true, true }, *scene);

    tree.rotateX(-glm::half_pi<float>()).setScale(0.1f);

    trc::Drawable floor({
        assets.create(trc::makePlaneGeo(50.0f, 50.0f, 60, 60)),
        assets.create(trc::MaterialData{
            .specularKoefficient=vec4(0.2f),
            .reflectivity=0.45f,
            .albedoTexture=assets.create(
                trc::loadTexture(TRC_TEST_ASSET_DIR"/rough_stone_wall.tif")
            ),
            .normalTexture=assets.create(
                trc::loadTexture(TRC_TEST_ASSET_DIR"/rough_stone_wall_normal.tif")
            ),
        }),
        false, true, true, true
    }, *scene);

    auto sun = scene->getLights().makeSunLight(vec3(1.0f), vec3(1, -1, -1), 0.5f);
    auto& shadow = scene->enableShadow(
        sun,
        { .shadowMapResolution=uvec2(2048, 2048) },
        torch->getShadowPool()
    );
    shadow.setProjectionMatrix(glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 10.0f));


    // --- Descriptor sets --- //

    vkb::DeviceLocalBuffer drawableBuf(
        device, scene->getRaySceneData(),
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vkb::DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    );

    vkb::Buffer instanceBuf(
        device, 500 * sizeof(trc::rt::GeometryInstance),
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
        | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
        vkb::DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    );
    auto data = instanceBuf.map<trc::rt::GeometryInstance*>();
    const size_t numInstances = scene->writeTlasInstances(data);
    instanceBuf.unmap();

    trc::rt::TLAS tlas(instance, numInstances);
    tlas.build(*instanceBuf, numInstances);

    auto tlasDescLayout = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eAccelerationStructureKHR, 1,
                    vk::ShaderStageFlagBits::eRaygenKHR)
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, trc::rt::ALL_RAY_PIPELINE_STAGE_FLAGS)
        .build(instance.getDevice());

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

    const auto tlasHandle = *tlas;
    vk::StructureChain asWriteChain{
        vk::WriteDescriptorSet(
            *tlasDescSet, 0, 0, 1,
            vk::DescriptorType::eAccelerationStructureKHR,
            {}, {}, {}
        ),
        vk::WriteDescriptorSetAccelerationStructureKHR(tlasHandle)
    };
    vk::DescriptorBufferInfo bufferInfo(*drawableBuf, 0, VK_WHOLE_SIZE);
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
        window,
        [&](ui32) {
            return trc::rt::RayBuffer(
                device,
                { window.getSize(), vk::ImageUsageFlagBits::eTransferSrc }
            );
        }
    };


    // --- Render Pass --- //

    auto& layout = torch->getRenderConfig().getLayout();

    // Add the pass that renders reflections to an offscreen image
    trc::RayTracingPass rayPass;
    layout.addPass(trc::rt::rayTracingRenderStage, rayPass);

    // Add the final compositing pass that merges rasterization and ray tracing results
    trc::rt::FinalCompositingPass compositing(
        window,
        trc::rt::FinalCompositingPassCreateInfo{
            .gBuffer = &torch->getRenderConfig().getGBuffer(),
            .rayBuffer = &rayBuffer,
            .assetRegistry = &assets.getDeviceRegistry()
        }
    );
    layout.addPass(trc::rt::finalCompositingStage, compositing);


    // --- Ray Pipeline --- //

    constexpr ui32 maxRecursionDepth{ 16 };
    auto rayPipelineLayout = trc::makePipelineLayout(device,
        {
            *tlasDescLayout,
            compositing.getInputImageDescriptor().getDescriptorSetLayout(),
            assets.getDeviceRegistry().getDescriptorSetProvider().getDescriptorSetLayout(),
            torch->getRenderConfig().getSceneDescriptorProvider().getDescriptorSetLayout(),
            torch->getShadowPool().getProvider().getDescriptorSetLayout(),
        },
        {
            // View and projection matrices
            { vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(mat4) * 2 },
        }
    );
    auto [rayPipeline, shaderBindingTable] =
        trc::rt::buildRayTracingPipeline(instance)
        .addRaygenGroup("/ray_tracing/reflect.rgen")
        .beginTableEntry()
            .addMissGroup("/ray_tracing/blue.rmiss")
        .endTableEntry()
        .addTrianglesHitGroup("/ray_tracing/reflect.rchit", "/ray_tracing/anyhit.rahit")
        .build(maxRecursionDepth, rayPipelineLayout);

    trc::DescriptorProvider tlasDescProvider{ *tlasDescLayout, *tlasDescSet };
    trc::FrameSpecificDescriptorProvider reflectionImageProvider(
        rayBuffer->getImageDescriptorLayout(),
        {
            window,
            [&](ui32 i) {
                return rayBuffer.getAt(i).getImageDescriptorSet(trc::rt::RayBuffer::eReflections);
            }
        }
    );
    auto& rayLayout = rayPipeline.getLayout();
    rayLayout.addStaticDescriptorSet(0, tlasDescProvider);
    rayLayout.addStaticDescriptorSet(1, compositing.getInputImageDescriptor());
    rayLayout.addStaticDescriptorSet(2, assets.getDeviceRegistry().getDescriptorSetProvider());
    rayLayout.addStaticDescriptorSet(3, torch->getRenderConfig().getSceneDescriptorProvider());
    rayLayout.addStaticDescriptorSet(4, torch->getShadowPool().getProvider());


    // --- Draw function --- //

    rayPass.addRayFunction(
        [
            &,
            &rayPipeline=rayPipeline,
            &shaderBindingTable=shaderBindingTable
        ](vk::CommandBuffer cmdBuf)
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
            cmdBuf.pushConstants<mat4>(
                *rayPipeline.getLayout(), vk::ShaderStageFlagBits::eRaygenKHR,
                0, { camera.getViewMatrix(), camera.getProjectionMatrix() }
            );

            cmdBuf.traceRaysKHR(
                shaderBindingTable.getEntryAddress(0),
                shaderBindingTable.getEntryAddress(1),
                shaderBindingTable.getEntryAddress(2),
                {},
                window.getImageExtent().width,
                window.getImageExtent().height,
                1,
                instance.getDL()
            );
        }
    );


    vkb::on<vkb::KeyPressEvent>([&](auto& e) {
        static bool count{ false };
        if (e.key == vkb::Key::r)
        {
            assets.getModule<trc::Material>().modify(
                floor.getMaterial().getDeviceID(),
                [](auto& mat) {
                    mat.reflectivity = 0.3f * float(count);
                }
            );
            count = !count;
        }
    });

    trc::Timer timer;
    trc::Timer frameTimer;
    int frames{ 0 };
    while (window.isOpen())
    {
        vkb::pollEvents();

        const float time = timer.reset();
        sphereNode.rotateY(time / 1000.0f * 0.5f);

        scene->update(time);

        auto data = instanceBuf.map<trc::rt::GeometryInstance*>();
        const size_t numInstances = scene->writeTlasInstances(data);
        instanceBuf.unmap();
        tlas.build(*instanceBuf, numInstances);

        window.drawFrame(torch->makeDrawConfig(*scene, camera));

        frames++;
        if (frameTimer.duration() >= 1000.0f)
        {
            std::cout << frames << " FPS\n";
            frames = 0;
            frameTimer.reset();
        }
    }

    window.getRenderer().waitForAllFrames();
}

int main()
{
    run();

    trc::terminate();

    return 0;
}
