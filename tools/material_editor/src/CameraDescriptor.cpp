#include "CameraDescriptor.h"

#include <trc/DescriptorSetUtils.h>



CameraDescriptor::CameraDescriptor(const trc::FrameClock& clock, const trc::Device& device)
    :
    clock(clock),
    buffer(
        device,
        sizeof(CameraMatrices) * clock.getFrameCount(),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible
        | vk::MemoryPropertyFlagBits::eHostCoherent
        | vk::MemoryPropertyFlagBits::eDeviceLocal
    ),
    bufferMap(buffer.map<CameraMatrices*>())
{
    auto builder = trc::buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex);

    layout = builder.build(device);
    pool = builder.buildPool(device, clock.getFrameCount(),
                             vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    set = std::move(device->allocateDescriptorSetsUnique({ *pool, *layout })[0]);

    // Write descriptor set
    vk::DescriptorBufferInfo bufferInfo{ *buffer, 0, sizeof(CameraMatrices) };
    vk::WriteDescriptorSet descWrite{
        *set, 0, 0,
        vk::DescriptorType::eUniformBufferDynamic,
        {}, bufferInfo, {}
    };
    device->updateDescriptorSets(descWrite, {});
}

void CameraDescriptor::update(const trc::Camera& camera)
{
    const ui32 frame = clock.getCurrentFrame();
    bufferMap[frame] = {
        .viewMatrix = camera.getViewMatrix(),
        .projMatrix = camera.getProjectionMatrix(),
        .inverseViewMatrix = glm::inverse(camera.getViewMatrix()),
        .inverseProjMatrix = glm::inverse(camera.getProjectionMatrix()),
    };
}

auto CameraDescriptor::getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout
{
    return *layout;
}

void CameraDescriptor::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    const ui32 frame = clock.getCurrentFrame();
    cmdBuf.bindDescriptorSets(
        bindPoint, pipelineLayout,
        setIndex,
        *set, frame * sizeof(CameraMatrices)
    );
}
