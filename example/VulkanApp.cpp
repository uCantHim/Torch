#include "VulkanApp.h"

#include <iostream>
#include <chrono>
using namespace std::chrono;
#include <filesystem>

#include <glm/gtc/matrix_transform.hpp>

#include "vkb/ShaderProgram.h"
#include "vkb/Image.h"

#include "RenderEnvironment.h"

#include "Settings.h"
#include "Vertex.h"
#include "TriangleList.h"
#include "Quad.h"



VulkanApp::VulkanApp()
    :
    descriptorPool([]() {
        std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1 }
        };
        return getDevice()->createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo{
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                10,
                static_cast<uint32_t>(descriptorPoolSizes.size()),
                descriptorPoolSizes.data()
            }
        );
    }()),

    standardDescriptorSetLayout([]() {
        std::vector<vk::DescriptorSetLayoutBinding> bindings = {
            vk::DescriptorSetLayoutBinding{
                DESCRIPTOR_BINDING_0,
                vk::DescriptorType::eUniformBuffer,
                1, // Array size
                vk::ShaderStageFlagBits::eVertex
            }
        };
        return getDevice()->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo{
                vk::DescriptorSetLayoutCreateFlags(),
                static_cast<uint32_t>(bindings.size()),
                bindings.data()
            }
        );
    }()),

    standardPushConstantRanges({
        vk::PushConstantRange{
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(mat4) * 3
        }
    }),

    standardPipelineLayout(getDevice()->createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo{
            vk::PipelineLayoutCreateFlags(),
            1, // Descriptor set layout count
            &*standardDescriptorSetLayout, // Descriptor set layouts
            static_cast<uint32_t>(standardPushConstantRanges.size()), // Push constant count
            standardPushConstantRanges.data() // Push constants
        }
    )),

    matrixBufferDescriptorSet(std::move(getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo{
            *descriptorPool,
            1, // Descriptor set count
            &*standardDescriptorSetLayout
        }
    )[0])),

    matrixBuffer(
        sizeof(mat4) * 3,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    )
{
    vkb::Image image("arch_3D_simplistic.png");
}


void VulkanApp::run()
{
    try {
        init();
        std::cout << "App initialized.\n";

        auto start = system_clock::now();
        int frames = 0;

        while (true) {
            tick();

            frames++;

            auto ms = duration_cast<milliseconds>(system_clock::now() - start).count();
            if (ms > 1000) {
                std::cout << frames << " frames in " << ms << " ms\n";
                std::cout << static_cast<float>(ms) / frames << " ms per frame\n\n";

                frames = 0;
                start = system_clock::now();
            }
        }

        getDevice()->waitIdle();

        destroy();
    }
    catch (const std::runtime_error& err) {
        std::cout << "An exception occured: " << err.what() << "\n";
        throw err;
    }
}


void VulkanApp::init()
{
    makeRenderEnvironment();

    for (size_t i = 0; i < TRIANGLELIST_COUNT; i++)
        scene.drawables.emplace_back(new TriangleList(TRIANGLELIST_VERTEX_COUNT));

    scene.drawables.emplace_back(new Quad);
}


void VulkanApp::tick()
{
    renderpass->drawFrame(scene);

    glfwPollEvents();
}


void VulkanApp::destroy()
{
}


void VulkanApp::makeRenderEnvironment()
{
    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription attachmentDescr(
        vk::AttachmentDescriptionFlags(),
        getSwapchain().getImageFormat(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    );

    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &colorAttachmentRef
    );

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL, 0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlags()
    );

    auto [newRenderpass, old] = Renderpass::create(
        STANDARD_RENDERPASS,
        {
            { attachmentDescr },
            { subpass },
            { dependency },
            {},
            { *matrixBufferDescriptorSet }
        }
    );
    renderpass = &newRenderpass;

    createPipeline(getSwapchain());

    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Standard rendering environment created.\n";
    }

    // Update buffers
    mat4 model{ glm::translate(mat4{1.0f}, vec3{0.0f, 0.0f, -2.0f})};
    mat4 view{ glm::lookAt(vec3{1.0f, 0.0f, 3.0f}, vec3{0.0f}, vec3{0.0f, 1.0f, 0.0f}) };
    auto imageExtent = getSwapchain().getImageExtent();
    mat4 proj{ glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(imageExtent.width) / static_cast<float>(imageExtent.height),
        1.0f, 100.0f)
    };

    auto buf = static_cast<uint8_t*>(matrixBuffer.map(0, VK_WHOLE_SIZE));
    memcpy(buf,                       &model[0][0], sizeof(mat4));
    memcpy(buf + sizeof(mat4),        &view[0][0], sizeof(mat4));
    memcpy(buf + sizeof(mat4) * 2, &proj[0][0], sizeof(mat4));
    matrixBuffer.unmap();

    vk::DescriptorBufferInfo bufferInfo{
        *matrixBuffer,
        0, // offset
        sizeof(mat4) * 3 // size
    };
    getDevice()->updateDescriptorSets(
        vk::WriteDescriptorSet{
            *matrixBufferDescriptorSet,
            DESCRIPTOR_BINDING_0,
            0, // dst array element
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr, // image info
            &bufferInfo,
            nullptr // texel buffer info
        },
        {} // descriptor copies
    );

    if constexpr (vkb::enableVerboseLogging)
        std::cout << "Matrix buffer updated\n";
}


void VulkanApp::createPipeline(vkb::Swapchain& swapchain)
{
    // Shader stages
    vkb::ShaderProgram shaders(
        "shaders/spirv_out/vertex.vert.spv",
        "shaders/spirv_out/fragment.frag.spv"
    );

    // Vertex input
    std::vector<vk::VertexInputBindingDescription> inputBindings = {
        { INPUT_BINDING_0, sizeof(Vertex), vk::VertexInputRate::eVertex }
    };
    std::vector<vk::VertexInputAttributeDescription> inputAttributes = {
        { ATTRIB_LOCATION_0, INPUT_BINDING_0, vk::Format::eR32G32B32Sfloat },
        { ATTRIB_LOCATION_1, INPUT_BINDING_0, vk::Format::eR32G32B32Sfloat },
        { ATTRIB_LOCATION_2, INPUT_BINDING_0, vk::Format::eR32G32Sfloat },
    };

    vk::PipelineVertexInputStateCreateInfo vertexInput(
        vk::PipelineVertexInputStateCreateFlags(),
        static_cast<uint32_t>(inputBindings.size()), inputBindings.data(),
        static_cast<uint32_t>(inputAttributes.size()), inputAttributes.data()
    );

    // Input assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
        vk::PipelineInputAssemblyStateCreateFlags(),
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE
    );

    // Tessellation
    vk::PipelineTessellationStateCreateInfo tessellation(
        vk::PipelineTessellationStateCreateFlags(),
        0u
    );

    // Viewport and scissor
    vk::Viewport vp(
        0.0f, 0.0f,
        static_cast<float>(swapchain.getImageExtent().width),
        static_cast<float>(swapchain.getImageExtent().height),
        0.0f, 1.0f
    );
    vk::Rect2D scissor(
        vk::Offset2D(0, 0),
        swapchain.getImageExtent()
    );
    vk::PipelineViewportStateCreateInfo viewport(
        vk::PipelineViewportStateCreateFlags(),
        1, &vp,
        1, &scissor
    );

    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer(
        vk::PipelineRasterizationStateCreateFlags(),
        VK_FALSE, VK_FALSE,                    // Depth clamp and rasterization discard
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE, 0.0f, 0.0f, 0.0f,            // Depth biases
        1.0f
    );

    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling(
        vk::PipelineMultisampleStateCreateFlags(),
        vk::SampleCountFlagBits::e1,
        VK_FALSE, 0.0f, nullptr,
        VK_FALSE, VK_FALSE
    );

    // Depth- and stencil tests
    vk::PipelineDepthStencilStateCreateInfo depthStencil(
        vk::PipelineDepthStencilStateCreateFlags(),
        VK_TRUE, VK_TRUE, vk::CompareOp::eGreater, VK_FALSE,
        VK_FALSE, vk::StencilOpState(), vk::StencilOpState(),
        0.0f, 0.0f
    );

    // Color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        VK_FALSE,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    vk::PipelineColorBlendStateCreateInfo colorBlending(
        vk::PipelineColorBlendStateCreateFlags(),
        VK_FALSE, vk::LogicOp::eCopy,
        1, &colorBlendAttachment
    );

    // Dynamic state
    vk::PipelineDynamicStateCreateInfo dynamicState(
        vk::PipelineDynamicStateCreateFlags(),
        0, nullptr
    );

    getDevice()->waitIdle();

    // Create the pipeline
    auto [pipeline, old] = GraphicsPipeline::create(
        STANDARD_PIPELINE,
        {
            shaders.getStages(),
            vertexInput,
            inputAssembly,
            tessellation,
            viewport,
            rasterizer,
            multisampling,
            depthStencil,
            colorBlending,
            dynamicState,
            *standardPipelineLayout,
            Renderpass::at(STANDARD_RENDERPASS), 0,
            vk::Pipeline(), 0,
            vk::PipelineCreateFlags()
        }
    );
    pipeline.addPipelineDescriptorSet(*matrixBufferDescriptorSet);
}


void VulkanApp::signalRecreateRequired()
{
}


void VulkanApp::recreate(vkb::Swapchain& swapchain)
{
    makeRenderEnvironment();
}


void VulkanApp::signalRecreateFinished()
{
}
