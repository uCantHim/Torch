#include <trc/DescriptorSetUtils.h>
#include <trc/Meshlet.h>
#include <trc/PipelineDefinitions.h>
#include <trc/RasterPlugin.h>
#include <trc/Torch.h>
#include <trc/TorchRenderStages.h>
#include <trc/core/PipelineBuilder.h>
#include <trc/core/PipelineLayoutBuilder.h>
#include <trc/drawable/DrawableScene.h>
using namespace trc::basic_types;

void run();
auto createPipeline() -> trc::Pipeline::ID;

int main()
{
    trc::init();
    run();
    trc::terminate();

    return 0;
}

void run()
{
    auto geo = trc::loadGeometry(TRC_TEST_ASSET_DIR"/sphere_many_triangles.fbx");

    trc::Timer timer;
    auto meshlets = trc::makeMeshletIndices(geo.indices);
    std::cout << "Meshlets representation created in " << timer.reset() << " ms\n";
    std::cout << meshlets.meshlets.size() << " meshlets with a total of "
        << meshlets.primitiveIndices.size() << " primitive vertices\n";

    const size_t indexSize = geo.indices.size() * sizeof(decltype(geo.indices.front()));
    const size_t meshletSize = meshlets.meshlets.size() * sizeof(trc::MeshletDescription)
        + meshlets.uniqueVertices.size() * sizeof(decltype(meshlets.uniqueVertices.front()))
        + meshlets.primitiveIndices.size() * sizeof(decltype(meshlets.primitiveIndices.front()));

    std::cout << "Size of index buffer: " << indexSize << "\n";
    std::cout << "Size of meshlet data: " << meshletSize << "\n";
    std::cout << "Compression rate: " << float(meshletSize) / float(indexSize) << "\n";



    auto torch = trc::initFull({}, trc::InstanceCreateInfo{
        .deviceExtensions={ VK_NV_MESH_SHADER_EXTENSION_NAME },
        .deviceFeatures={ vk::PhysicalDeviceMeshShaderFeaturesNV{} },
    });
    const trc::Device& device = torch->getDevice();
    trc::Window& window = torch->getWindow();

    auto descLayout = trc::buildDescriptorSetLayout()
        // Meshlet descriptions
        .addBinding(vk::DescriptorType::eUniformTexelBuffer, 1, vk::ShaderStageFlagBits::eMeshNV)
        // Unique vertex indices
        .addBinding(vk::DescriptorType::eUniformTexelBuffer, 1, vk::ShaderStageFlagBits::eMeshNV)
        // Primitive indices
        .addBinding(vk::DescriptorType::eUniformTexelBuffer, 1, vk::ShaderStageFlagBits::eMeshNV)
        // Vertices
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eMeshNV)
        .build(device);

    torch->getRenderPipeline().getResourceConfig().defineDescriptor(
        trc::DescriptorName{ "mesh_input" },
        *descLayout
    );

    trc::DeviceLocalBuffer meshletDescBuffer(
        device, meshlets.meshlets, vk::BufferUsageFlagBits::eUniformTexelBuffer
    );
    trc::DeviceLocalBuffer vertexIndexBuffer(
        device, meshlets.uniqueVertices, vk::BufferUsageFlagBits::eUniformTexelBuffer
    );
    trc::DeviceLocalBuffer primitiveIndexBuffer(
        device, meshlets.primitiveIndices, vk::BufferUsageFlagBits::eUniformTexelBuffer
    );
    trc::DeviceLocalBuffer vertexBuffer(
        device, geo.vertices, vk::BufferUsageFlagBits::eStorageBuffer
    );

    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eUniformTexelBuffer, 3 },
    };
    auto descPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes
    ));
    auto set = std::move(device->allocateDescriptorSetsUnique({ *descPool, *descLayout })[0]);
    std::vector<vk::DescriptorBufferInfo> bufferInfos{
        { *vertexBuffer, 0, VK_WHOLE_SIZE },
    };
    std::vector<vk::UniqueBufferView> bufferViews;
    bufferViews.emplace_back(device->createBufferViewUnique({
        {}, *meshletDescBuffer, vk::Format::eR32G32B32A32Uint, 0, VK_WHOLE_SIZE
    }));
    bufferViews.emplace_back(device->createBufferViewUnique({
        {}, *vertexIndexBuffer, vk::Format::eR32Uint, 0, VK_WHOLE_SIZE
    }));
    bufferViews.emplace_back(device->createBufferViewUnique({
        {}, *primitiveIndexBuffer, vk::Format::eR32G32Uint, 0, VK_WHOLE_SIZE
    }));

    std::vector<vk::WriteDescriptorSet> writes{
        vk::WriteDescriptorSet(*set, 0, 0, vk::DescriptorType::eUniformTexelBuffer, {}, {}, *bufferViews[0]),
        vk::WriteDescriptorSet(*set, 1, 0, vk::DescriptorType::eUniformTexelBuffer, {}, {}, *bufferViews[1]),
        vk::WriteDescriptorSet(*set, 2, 0, vk::DescriptorType::eUniformTexelBuffer, {}, {}, *bufferViews[2]),
        vk::WriteDescriptorSet(*set, 3, 0, vk::DescriptorType::eStorageBuffer,      {}, bufferInfos[0]),
    };
    device->updateDescriptorSets(writes, {});



    // Camera setup
    trc::Camera camera;
    camera.lookAt(vec3(0, 1, 2.5f), vec3(0, 0.5f, 0), vec3(0, 1, 0));
    camera.makePerspective(float(window.getSize().x) / float(window.getSize().y), 45.0f, 0.1f, 100.0f);

    // Scene setup
    trc::DescriptorProvider meshInputProvider{ {} };
    trc::DrawableScene scene;
    auto sun = scene.getLights().makeSunLight(vec3(1.0f), vec3(1.0f, -0.3f, 0), 0.3f);

    // Object properties
    mat4 modelMatrix = trc::Transformation{}.setScale(0.9f).translateY(0.5f).getTransformationMatrix();

    // Mesh draw function
    auto pipeline = createPipeline();
    scene.getRasterModule().registerDrawFunction(trc::gBufferRenderStage, trc::SubPass::ID(0), pipeline,
        [&](const trc::DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            auto layout = *env.currentPipeline->getLayout();
            cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, *set, {});
            cmdBuf.pushConstants<mat4>(layout, vk::ShaderStageFlagBits::eMeshNV, 0, modelMatrix);
            cmdBuf.drawMeshTasksNV(meshlets.meshlets.size(), 0, torch->getInstance().getDL());
        }
    );

    while (window.isOpen())
    {
        trc::pollEvents();
        torch->drawFrame(camera, scene);
    }

    torch->waitForAllFrames();
}

auto createPipeline() -> trc::Pipeline::ID
{
    auto layout = trc::buildPipelineLayout()
        .addDescriptor(trc::DescriptorName{ "mesh_input" }, false)
        .addDescriptor(trc::DescriptorName{ trc::RasterPlugin::GLOBAL_DATA_DESCRIPTOR }, true)
        .addPushConstantRange({ vk::ShaderStageFlagBits::eMeshNV, 0, sizeof(mat4) + sizeof(ui32) })
        .registerLayout();

    return trc::buildGraphicsPipeline()
        .setMeshShadingProgram(
            std::nullopt, trc::internal::loadShader(trc::ShaderPath("/mesh_shading.mesh")),
            trc::internal::loadShader(trc::ShaderPath("/mesh_shading.frag"))
        )
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .disableBlendAttachments(3)
        .registerPipeline(
            layout,
            trc::RenderPassName{ trc::RasterPlugin::OPAQUE_G_BUFFER_PASS }
        );
}
