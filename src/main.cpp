#include <chrono>
using namespace std::chrono;
#include <iostream>
#include <fstream>

#include <vkb/VulkanBase.h>
#include <vkb/Buffer.h>
#include <vkb/Image.h>
#include <vkb/ShaderProgram.h>
#include <vkb/FrameSpecificObject.h>

#include "trc/base/SceneBase.h"
#include "trc/base/DrawableStatic.h"
#include "trc/utils/Transformation.h"
#include "trc/utils/FBXLoader.h"
#include "trc/Geometry.h"
#include "trc/CommandCollector.h"
#include "trc/PipelineBuilder.h"

std::ofstream file("trash");

/**
 * @brief A sample implementation
 */
class Thing : public trc::SceneRegisterable,
              public trc::StaticPipelineRenderInterface<Thing, 0, 0>,
              public trc::StaticPipelineRenderInterface<Thing, 0, 1>,
              public trc::StaticPipelineRenderInterface<Thing, 0, 2>
{
public:
    void recordCommandBuffer(trc::PipelineIndex<0>, vk::CommandBuffer)
    {
        file << "_";
    }

    void recordCommandBuffer(trc::PipelineIndex<1>, vk::CommandBuffer)
    {
        file << "_";
    }

    void recordCommandBuffer(trc::PipelineIndex<2>, vk::CommandBuffer)
    {
        file << "_";
    }
};


constexpr uint32_t DESCRIPTOR_BINDING_0 = 0;


void createRenderpass();
void createPipeline(trc::PipelineLayout& pipelineLayout);


int main()
{
    using v = vkb::VulkanBase;
    vkb::vulkanInit({});

    trc::FBXLoader fbxLoader;
    trc::Geometry geo(fbxLoader.loadFBXFile("grass_lowpoly.fbx").meshes[0].mesh);
    std::cin.get();

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
    trc::PipelineLayout pipelineLayout(
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
                    *trc::RenderPass::at(0),
                    static_cast<uint32_t>(imageViews.size()), imageViews.data(),
                    swapchain.getImageExtent().width, swapchain.getImageExtent().height,
                    1 // layers
                )
            );
        }
    );

    trc::CommandCollector engine;
    trc::SceneBase scene;

    constexpr size_t NUM_OBJECTS = 2000;

    std::vector<Thing> objects(NUM_OBJECTS);
    for (auto& obj : objects)
    {
        obj.attachToScene(scene);
    }

    while (true)
    {
        auto start = system_clock::now();

        trc::DrawInfo info{
            .renderPass=&trc::RenderPass::at(0),
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

        auto end = system_clock::now();
        std::cout << "Frame duration (" << NUM_OBJECTS << " objects): "
            << duration_cast<microseconds>(end - start).count() << " µs\n";
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
        trc::makeDefaultSwapchainColorAttachment(vkb::VulkanBase::getSwapchain()),
        trc::makeDefaultDepthStencilAttachment(),
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

    trc::RenderPass::create(
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


void createPipeline(trc::PipelineLayout& pipelineLayout)
{
    struct Vertex
    {
        vec3 pos;
        vec3 normal;
        vec2 uv;
    };

    const auto& swapchain = vkb::VulkanBase::getSwapchain();

    // Shader stages
    vkb::ShaderProgram program(
        "shaders/spirv_out/vertex.vert.spv",
        "shaders/spirv_out/fragment.frag.spv"
    );

    auto builder = trc::GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addVertexInputBinding(
            { 0, sizeof(Vertex), vk::VertexInputRate::eVertex },
            {
                { 0, 0, vk::Format::eR32G32B32Sfloat },
                { 1, 0, vk::Format::eR32G32B32Sfloat },
                { 2, 0, vk::Format::eR32G32Sfloat },
            }
        )
        .addViewport(
            vk::Viewport(
                0.0f, 0.0f,
                static_cast<float>(swapchain.getImageExtent().width),
                static_cast<float>(swapchain.getImageExtent().height),
                0.0f, 1.0f
            )
        )
        .addScissorRect(
            vk::Rect2D(
                vk::Offset2D(0, 0),
                swapchain.getImageExtent()
            )
        )
        .addColorBlendAttachment(
            trc::DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED
        )
        .setColorBlending({}, VK_FALSE, vk::LogicOp::eCopy, {});

    for (int i = 0; i < 3; i++)
    {
        trc::GraphicsPipeline::create(
            i,
            *pipelineLayout,
            builder.build(*vkb::VulkanBase::getDevice(), *pipelineLayout, *trc::RenderPass::at(0), 0));
    }
}
