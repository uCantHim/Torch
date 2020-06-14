#include <iostream>

#include <glm/glm.hpp>
using namespace glm;

#include <vkb/VulkanBase.h>
#include <vkb/Buffer.h>
#include <vkb/Image.h>
#include <vkb/ShaderProgram.h>

#include "CommandCollector.h"
#include "Scene.h"
#include "StaticDrawable.h"

/**
 * @brief A sample implementation
 */
class Thing : public SceneRegisterable,
              public StaticPipelineRenderInterface<Thing, 0, 0>
{
public:
    void recordCommandBuffer(PipelineIndex<0>, vk::CommandBuffer)
    {
        std::cout << "Recording commands for pipeline 0...\n";
    }
};


constexpr uint32_t DESCRIPTOR_BINDING_0 = 0;


void createRenderpass();
void createPipeline(PipelineLayout& pipelineLayout);


int main()
{
    using v = vkb::VulkanBase;
    vkb::vulkanInit({});

    auto descriptorPool = []() -> vk::UniqueDescriptorPool {
        std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 1 },
            vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 },
        };
        return v::getDevice()->createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo{
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                10,
                static_cast<uint32_t>(descriptorPoolSizes.size()),
                descriptorPoolSizes.data()
            }
        );
    }();

    auto standardDescriptorSetLayout = []() -> vk::UniqueDescriptorSetLayout {
        std::vector<vk::DescriptorSetLayoutBinding> bindings = {
            {
                DESCRIPTOR_BINDING_0,
                vk::DescriptorType::eUniformBuffer,
                1, // Array size
                vk::ShaderStageFlagBits::eVertex
            },
            {
                1,
                vk::DescriptorType::eCombinedImageSampler,
                1,
                vk::ShaderStageFlagBits::eFragment
            },
        };
        return v::getDevice()->createDescriptorSetLayoutUnique(
            vk::DescriptorSetLayoutCreateInfo{
                vk::DescriptorSetLayoutCreateFlags(),
                static_cast<uint32_t>(bindings.size()),
                bindings.data()
            }
        );
    }();

    std::vector<vk::PushConstantRange> standardPushConstantRanges{
        vk::PushConstantRange{
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(mat4) * 3
        }
    };
    PipelineLayout pipelineLayout(
        std::vector<vk::DescriptorSetLayout>{ *standardDescriptorSetLayout },
        standardPushConstantRanges
    );

    auto matrixBufferDescriptorSet = std::move(
        v::getDevice()->allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo{
                *descriptorPool,
                1, // Descriptor set count
                &*standardDescriptorSetLayout
            }
    )[0]);

    vkb::Buffer matrixBuffer(
        sizeof(mat4) * 3,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    vkb::Image texture("arch_3D_simplistic.png");
    auto textureImageView = texture.createView(
        vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Snorm, {}
    );
    texture.changeLayout(vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal);

    const auto& swapchain = vkb::VulkanBase::getSwapchain();
    createRenderpass();
    createPipeline(pipelineLayout);

    vkb::FrameSpecificObject<vk::UniqueImageView> colorAttachmentImageViews(
        vkb::VulkanBase::getSwapchain().createImageViews()
    );

    vkb::FrameSpecificObject<vkb::Image> depthImages(
        [&swapchain](uint32_t) {
            return vkb::Image(vk::ImageCreateInfo(
                {},
                vk::ImageType::e2D,
                vk::Format::eD24UnormS8Uint,
                vk::Extent3D{ swapchain.getImageExtent(), 1 },
                1, 1, vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment
            ));
        }
    );
    vkb::FrameSpecificObject<vk::UniqueImageView> depthAttachmentImageViews(
        [&depthImages](uint32_t imageIndex) {
            return depthImages.getAt(imageIndex).createView(
                vk::ImageViewType::e2D, vk::Format::eD24UnormS8Uint, vk::ComponentMapping(),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
            );
        }
    );

    vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers(
        [&](uint32_t imageIndex) -> vk::UniqueFramebuffer
        {
            std::vector<vk::ImageView> imageViews;
            imageViews.push_back(*colorAttachmentImageViews.getAt(imageIndex));
            imageViews.push_back(*depthAttachmentImageViews.getAt(imageIndex));

            return vkb::VulkanBase::getDevice()->createFramebufferUnique(
                vk::FramebufferCreateInfo(
                    {},
                    *RenderPass::at(0),
                    static_cast<uint32_t>(imageViews.size()), imageViews.data(),
                    swapchain.getImageExtent().width, swapchain.getImageExtent().height,
                    1 // layers
                )
            );
        }
    );

    CommandCollector engine;
    Scene scene;

    scene.registerDrawFunction(0, 0, [](vk::CommandBuffer) {
        std::cout << "Random draw function\n";
    });

    Thing thing;
    thing.attachToScene(scene);

    while (true)
    {
        DrawInfo info{
            .renderPass=&RenderPass::at(0),
            .framebuffer=**framebuffers,
            .viewport={
                { 0, 0 },
                { swapchain.getImageExtent().width, swapchain.getImageExtent().height }
            }
        };
        auto cmdBuf = engine.recordScene(scene, info);

        // Submit work and present the image
        auto semaphore = vkb::VulkanBase::getDevice()->createSemaphoreUnique(
            vk::SemaphoreCreateInfo(vk::SemaphoreCreateFlags())
        );
        auto image = swapchain.acquireImage(*semaphore);
        vkb::VulkanBase::getDevice().executeGraphicsCommandBufferSynchronously(cmdBuf);
        vkb::VulkanBase::getSwapchain().presentImage(
            image,
            vkb::VulkanBase::getQueueProvider().getQueue(vkb::queue_type::presentation),
            {}
        );

    }

    std::cout << " --- Done\n";
    return 0;
}


void createRenderpass()
{
    std::vector<vk::AttachmentReference> attachmentRefs = {
        { 0, vk::ImageLayout::eColorAttachmentOptimal },
        { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal },
    };

    std::vector<vk::AttachmentDescription> attachments = {
        makeDefaultSwapchainColorAttachment(vkb::VulkanBase::getSwapchain()),
        makeDefaultDepthStencilAttachment(),
    };

    vk::SubpassDescription subpass(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &attachmentRefs[0],
        nullptr, // resolve attachments
        &attachmentRefs[1]
    );

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL, 0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlags()
    );

    RenderPass::create(
        0,
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            static_cast<uint32_t>(attachments.size()), attachments.data(),
            1u, &subpass,
            1u, &dependency
        ),
        std::vector<vk::ClearValue>{
            vk::ClearColorValue(std::array<float, 4>{ 1.0, 0.4, 1.0 }),
            vk::ClearDepthStencilValue(1.0f, 0),
        }
    );
}


void createPipeline(PipelineLayout& pipelineLayout)
{
    struct Vertex
    {
        vec3 pos;
        vec3 normal;
        vec2 uv;
    };

    const auto& swapchain = vkb::VulkanBase::getSwapchain();

    // Shader stages
    vkb::ShaderProgram shaders(
        "shaders/spirv_out/vertex.vert.spv",
        "shaders/spirv_out/fragment.frag.spv"
    );

    // Vertex input
    std::vector<vk::VertexInputBindingDescription> inputBindings = {
        { 0, sizeof(Vertex), vk::VertexInputRate::eVertex }
    };
    std::vector<vk::VertexInputAttributeDescription> inputAttributes = {
        { 0, 0, vk::Format::eR32G32B32Sfloat },
        { 1, 0, vk::Format::eR32G32B32Sfloat },
        { 2, 0, vk::Format::eR32G32Sfloat },
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
        VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE,
        VK_FALSE, vk::StencilOpState(), vk::StencilOpState(),
        0.0f, 1.0f
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

    vkb::VulkanBase::getDevice()->waitIdle();

    // Create the pipeline
    GraphicsPipeline::create(
        0,
        vk::GraphicsPipelineCreateInfo(
            {},
            static_cast<uint32_t>(shaders.getStages().size()), shaders.getStages().data(),
            &vertexInput,
            &inputAssembly,
            &tessellation,
            &viewport,
            &rasterizer,
            &multisampling,
            &depthStencil,
            &colorBlending,
            &dynamicState,
            *pipelineLayout,
            *RenderPass::at(0), 0,
            vk::Pipeline(), 0
        ),
        pipelineLayout
    );
}
