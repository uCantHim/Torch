#include "trc/RenderDataDescriptor.h"

#include "trc_util/Util.h"
#include "trc/core/Window.h"



trc::GlobalRenderDataDescriptor::GlobalRenderDataDescriptor(
    const Device& device,
    ui32 maxDescriptorSets)
    :
    device(device),
    kBufferSectionSize(util::pad(
        kCameraDataSize + kViewportDataSize,
        device.getPhysicalDevice().properties.limits.minUniformBufferOffsetAlignment
    )),
    kMaxDescriptorSets(maxDescriptorSets),
    buffer(
        device,
        vk::DeviceSize{kBufferSectionSize * maxDescriptorSets},
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

auto trc::GlobalRenderDataDescriptor::makeDescriptorSet() const -> DescriptorSet
{
    static ui32 nextIndex{ 0 };

    const ui32 currentSetIndex = nextIndex;
    nextIndex = (nextIndex + 1) % kMaxDescriptorSets;
    const ui32 offset = currentSetIndex * kBufferSectionSize;

    auto descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, *descLayout })[0]);

    // Update descriptor set
    std::vector<vk::DescriptorBufferInfo> buffers{
        { *buffer, offset,                   kCameraDataSize },
        { *buffer, offset + kCameraDataSize, kViewportDataSize },
    };
    std::vector<vk::WriteDescriptorSet> writes{
        vk::WriteDescriptorSet(
            *descSet, 0, 0,
            vk::DescriptorType::eUniformBuffer,
            nullptr, buffers, nullptr
        ),
    };
    device->updateDescriptorSets(writes, {});

    return { this, std::move(descSet), offset };
}

void trc::GlobalRenderDataDescriptor::createDescriptors()
{
    // Create descriptors
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eUniformBuffer, 2 }
    };
    descPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            kMaxDescriptorSets,
            poolSizes
    ));

    vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eAllGraphics
                                      | vk::ShaderStageFlagBits::eCompute
                                      | vk::ShaderStageFlagBits::eRaygenKHR;
    if (device.getPhysicalDevice().getFeature<vk::PhysicalDeviceMeshShaderFeaturesNV>().meshShader) {
        shaderStages |= vk::ShaderStageFlagBits::eMeshNV | vk::ShaderStageFlagBits::eTaskNV;
    }
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        { 0, vk::DescriptorType::eUniformBuffer, 1, shaderStages },
        { 1, vk::DescriptorType::eUniformBuffer, 1, shaderStages },
    };

    descLayout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({},
        layoutBindings
    ));
}

void trc::GlobalRenderDataDescriptor::DescriptorSet::update(const Camera& camera)
{
    auto& buffer = parent->buffer;

    // Camera matrices
    auto mats = buffer.map<mat4*>(bufferOffset, kCameraDataSize);
    mats[0] = camera.getViewMatrix();
    mats[1] = camera.getProjectionMatrix();
    mats[2] = inverse(camera.getViewMatrix());
    mats[3] = inverse(camera.getProjectionMatrix());
    buffer.unmap();

    // Resolution and mouse pos
    auto buf = buffer.map<vec2*>(
        bufferOffset + kCameraDataSize, kViewportDataSize
    );
    buf[0] = vec2(0.0f);  // swapchain.getMousePosition();
    buf[1] = vec2(0.0f);  // vec2(swapchain.getSize());
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::DescriptorSet::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex
    ) const
{
    cmdBuf.bindDescriptorSets(bindPoint, pipelineLayout, setIndex, *descSet, {});
}
