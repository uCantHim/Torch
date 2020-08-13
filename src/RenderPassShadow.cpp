#include "RenderPassShadow.h"

#include "PipelineDefinitions.h"



trc::SunShadowPass::SunShadowPass(uvec2 resolution, const Light& light)
    :
    RenderPassShadow(
        [&]()
        {
            std::vector<vk::AttachmentDescription> attachments{
                vk::AttachmentDescription(
                    {},
                    vk::Format::eD24UnormS8Uint,
                    vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal
                )
            };

            vk::AttachmentReference depthRef(0, vk::ImageLayout::eDepthStencilAttachmentOptimal);
            std::vector<vk::SubpassDescription> subpasses{
                vk::SubpassDescription(
                    {},
                    vk::PipelineBindPoint::eGraphics,
                    0, nullptr,
                    0, nullptr,
                    nullptr,
                    &depthRef
                ),
            };

            std::vector<vk::SubpassDependency> dependencies{
                vk::SubpassDependency(
                    VK_SUBPASS_EXTERNAL, 0,
                    vk::PipelineStageFlagBits::eAllCommands,
                    vk::PipelineStageFlagBits::eAllGraphics,
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                    vk::DependencyFlagBits::eByRegion
                ),
            };

            return vkb::getDevice()->createRenderPassUnique(
                vk::RenderPassCreateInfo({}, attachments, subpasses, dependencies)
            );
        }(),
        1
    ),
    resolution(resolution),
    light(&light),
    depthImages([&resolution](ui32) {
        return vkb::Image(
            vk::ImageCreateInfo(
                {},
                vk::ImageType::e2D,
                vk::Format::eD24UnormS8Uint,
                vk::Extent3D(resolution.x, resolution.y, 1),
                1, 1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment
            )
        );
    }),
    depthImageViews([&](ui32 imageIndex) {
        return depthImages.getAt(imageIndex).createView(
            vk::ImageViewType::e2D, vk::Format::eD24UnormS8Uint, {},
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
        );
    }),
    framebuffers([&](ui32 imageIndex) {
        vk::ImageView depthView = *depthImageViews.getAt(imageIndex);
        return vkb::getDevice()->createFramebufferUnique(
            vk::FramebufferCreateInfo(
                {},
                *renderPass,
                depthView,
                resolution.x, resolution.y, 1
            )
        );
    })
{
}

void trc::SunShadowPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents)
{
    cmdBuf.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            **framebuffers,
            vk::Rect2D({ 0, 0 }, { resolution.x, resolution.y }),
            0, nullptr
        ),
        subpassContents
    );
}

void trc::SunShadowPass::end(vk::CommandBuffer cmdBuf)
{
    cmdBuf.endRenderPass();
}

auto trc::SunShadowPass::getResolution() const noexcept -> uvec2
{
    return resolution;
}



auto trc::ShadowDescriptor::getProvider() noexcept -> const DescriptorProviderInterface&
{
    return *descProvider;
}

void trc::ShadowDescriptor::addShadow(vk::Image image, const mat4& viewMatrix, const mat4& proj)
{

}

void trc::ShadowDescriptor::init()
{
    descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        vkb::getSwapchain().getFrameCount(),
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 1 },
            { vk::DescriptorType::eCombinedImageSampler, MAX_SHADOW_MAPS },
        }
    ));

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        // Shadow matrix buffer
        vk::DescriptorSetLayoutBinding(
            0, vk::DescriptorType::eStorageBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        ),
        // Shadow samplers
        vk::DescriptorSetLayoutBinding(
            1, vk::DescriptorType::eCombinedImageSampler, MAX_SHADOW_MAPS,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        ),
    };
    std::vector<vk::DescriptorBindingFlags> bindingFlags{
        {},
        vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    };
    vk::StructureChain layoutChain{
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
            layoutBindings
        ),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(bindingFlags)
    };
    descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    // Sets
    std::vector<vk::DescriptorSetLayout> layouts;
    for (int i = 0; i < vkb::getSwapchain().getFrameCount(); i++) {
        layouts.push_back(*descLayout);
    }
    descSet = std::make_unique<vkb::FrameSpecificObject<vk::UniqueDescriptorSet>>(
        vkb::getDevice()->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(
            *descPool,
            vkb::getSwapchain().getFrameCount(),
            layouts.data()
        ))
    );

    vkb::FrameSpecificObject<vk::DescriptorSet> sets{ [](ui32 i) { return *descSet->getAt(i); } };
    descProvider = std::make_unique<FrameSpecificDescriptorProvider>(*descLayout, std::move(sets));
}

void trc::ShadowDescriptor::destroy()
{
    descSet.reset();
    descLayout.reset();
    descPool.reset();
}
