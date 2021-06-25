#include "RenderDataDescriptor.h"

#include "utils/Util.h"



trc::GlobalRenderDataDescriptor::GlobalRenderDataDescriptor()
    :
    provider(*this),
    BUFFER_SECTION_SIZE(util::pad(
        CAMERA_DATA_SIZE + SWAPCHAIN_DATA_SIZE,
        vkb::getPhysicalDevice().properties.limits.minUniformBufferOffsetAlignment
    ))
{
    createResources();
    createDescriptors();
}

auto trc::GlobalRenderDataDescriptor::getProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return provider;
}

void trc::GlobalRenderDataDescriptor::updateCameraMatrices(const Camera& camera)
{
    auto buf = reinterpret_cast<mat4*>(buffer.map(
        BUFFER_SECTION_SIZE * vkb::getSwapchain().getCurrentFrame(), CAMERA_DATA_SIZE
    ));
    buf[0] = camera.getViewMatrix();
    buf[1] = camera.getProjectionMatrix();
    buf[2] = inverse(camera.getViewMatrix());
    buf[3] = inverse(camera.getProjectionMatrix());
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::updateSwapchainData(const vkb::Swapchain& swapchain)
{
    const ui32 currentFrame = vkb::getSwapchain().getCurrentFrame();
    const vec2 res = vec2(static_cast<float>(swapchain.getImageExtent().width),
                          static_cast<float>(swapchain.getImageExtent().height));

    auto buf = reinterpret_cast<vec2*>(buffer.map(
        BUFFER_SECTION_SIZE * currentFrame + CAMERA_DATA_SIZE, SWAPCHAIN_DATA_SIZE
    ));
    buf[0] = swapchain.getMousePosition();
    buf[1] = res;
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::createResources()
{
    buffer = vkb::Buffer(
        BUFFER_SECTION_SIZE * vkb::getSwapchain().getFrameCount(),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
}

void trc::GlobalRenderDataDescriptor::createDescriptors()
{
    // Create descriptors
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eUniformBufferDynamic, 2 }
    };
    descPool = vkb::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            vkb::getSwapchain().getFrameCount(),
            poolSizes
    ));

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        {
            0, vk::DescriptorType::eUniformBufferDynamic, 1,
            vk::ShaderStageFlagBits::eAllGraphics
        },
        {
            1, vk::DescriptorType::eUniformBufferDynamic, 1,
            vk::ShaderStageFlagBits::eAllGraphics
        },
    };
    descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({},
        layoutBindings
    ));

    descSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    // Update descriptor set
    vk::DescriptorBufferInfo cameraInfo(*buffer, 0, CAMERA_DATA_SIZE);
    vk::DescriptorBufferInfo swapchainInfo(*buffer, CAMERA_DATA_SIZE, SWAPCHAIN_DATA_SIZE);
    std::vector<vk::WriteDescriptorSet> writes{
        vk::WriteDescriptorSet(
            *descSet, 0, 0, 1,
            vk::DescriptorType::eUniformBufferDynamic,
            nullptr, &cameraInfo, nullptr
        ),
        vk::WriteDescriptorSet(
            *descSet, 1, 0, 1,
            vk::DescriptorType::eUniformBufferDynamic,
            nullptr, &swapchainInfo, nullptr
        ),
    };
    vkb::getDevice()->updateDescriptorSets(writes, {});
}

trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::RenderDataDescriptorProvider(
    const GlobalRenderDataDescriptor& desc)
    :
    descriptor(desc)
{
}

auto trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::getDescriptorSet()
    const noexcept -> vk::DescriptorSet
{
    return *descriptor.descSet;
}

auto trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::getDescriptorSetLayout()
    const noexcept -> vk::DescriptorSetLayout
{
    return *descriptor.descLayout;
}

void trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    const ui32 dynamicOffset = descriptor.BUFFER_SECTION_SIZE
                               * vkb::getSwapchain().getCurrentFrame();

    cmdBuf.bindDescriptorSets(
        bindPoint, pipelineLayout, setIndex,
        *descriptor.descSet,
        { dynamicOffset, dynamicOffset }
    );
}
