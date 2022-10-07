#include "RenderDataDescriptor.h"

#include "trc_util/Util.h"
#include "trc/core/Window.h"



trc::GlobalRenderDataDescriptor::GlobalRenderDataDescriptor(const Window& window)
    :
    device(window.getDevice()),
    swapchain(window.getSwapchain()),
    BUFFER_SECTION_SIZE(util::pad(
        CAMERA_DATA_SIZE + SWAPCHAIN_DATA_SIZE,
        device.getPhysicalDevice().properties.limits.minUniformBufferOffsetAlignment
    )),
    buffer(
        window.getDevice(),
        BUFFER_SECTION_SIZE * swapchain.getFrameCount(),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    )
{
    createDescriptors();
}

auto trc::GlobalRenderDataDescriptor::getDescriptorSetLayout()
    const noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::GlobalRenderDataDescriptor::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    const ui32 dynamicOffset = BUFFER_SECTION_SIZE * swapchain.getCurrentFrame();
    cmdBuf.bindDescriptorSets(
        bindPoint, pipelineLayout,
        setIndex, *descSet,
        { dynamicOffset, dynamicOffset }
    );
}

void trc::GlobalRenderDataDescriptor::update(const Camera& camera)
{
    const ui32 currentFrame = swapchain.getCurrentFrame();

    // Camera matrices
    auto mats = buffer.map<mat4*>(BUFFER_SECTION_SIZE * currentFrame, CAMERA_DATA_SIZE);
    mats[0] = camera.getViewMatrix();
    mats[1] = camera.getProjectionMatrix();
    mats[2] = inverse(camera.getViewMatrix());
    mats[3] = inverse(camera.getProjectionMatrix());
    buffer.unmap();

    // Resolution and mouse pos
    auto buf = buffer.map<vec2*>(
        BUFFER_SECTION_SIZE * currentFrame + CAMERA_DATA_SIZE, SWAPCHAIN_DATA_SIZE
    );
    buf[0] = swapchain.getMousePosition();
    buf[1] = vec2(swapchain.getSize());
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::createDescriptors()
{
    // Create descriptors
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eUniformBufferDynamic, 2 }
    };
    descPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            swapchain.getFrameCount(),
            poolSizes
    ));

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        {
            0, vk::DescriptorType::eUniformBufferDynamic, 1,
            vk::ShaderStageFlagBits::eAllGraphics
                | vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eRaygenKHR
        },
        {
            1, vk::DescriptorType::eUniformBufferDynamic, 1,
            vk::ShaderStageFlagBits::eAllGraphics
                | vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eRaygenKHR
        },
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({},
        layoutBindings
    ));

    descSet = std::move(device->allocateDescriptorSetsUnique(
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
    device->updateDescriptorSets(writes, {});
}
