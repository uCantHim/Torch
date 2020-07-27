#include "Animation.h"



trc::Animation::Animation(const AnimationData& data)
    :
    animationIndex(numAnimations),
    frameCount(data.frameCount),
    durationMs(data.durationMs),
    frameTimeMs(data.frameTimeMs)
{
    std::lock_guard lock(animationCreateLock);

    AnimationMeta newMeta{
        .offset = animationBufferOffset,
        .frameCount = data.frameCount,
        .boneCount = static_cast<ui32>(data.keyframes[0].boneMatrices.size())
    };
    auto metaBuf = animationMetaDataBuffer.map(numAnimations * sizeof(AnimationMeta));
    memcpy(metaBuf, &newMeta, sizeof(newMeta));
    animationMetaDataBuffer.unmap();

    auto animBuf = animationBuffer.map(animationBufferOffset * sizeof(mat4));
    for (size_t offset = 0; const auto& kf : data.keyframes)
    {
        memcpy(animBuf + offset, kf.boneMatrices.data(), kf.boneMatrices.size() * sizeof(mat4));
        offset += kf.boneMatrices.size() * sizeof(mat4);
    }
    animationBuffer.unmap();

    numAnimations++;
    animationBufferOffset += data.frameCount * newMeta.boneCount;
}

auto trc::Animation::getDescriptorProvider() noexcept -> DescriptorProviderInterface&
{
    return descProvider;
}

auto trc::Animation::getBufferIndex() const noexcept -> ui32
{
    return animationIndex;
}

auto trc::Animation::getFrameCount() const noexcept -> ui32
{
    return frameCount;
}

auto trc::Animation::getDuration() const noexcept -> float
{
    return durationMs;
}

auto trc::Animation::getFrameTime() const noexcept -> float
{
    return frameTimeMs;
}

void trc::Animation::vulkanStaticInit()
{
    animationMetaDataBuffer = vkb::Buffer(
        MAX_ANIMATIONS * sizeof(AnimationMeta),
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    animationBuffer = vkb::Buffer(
        ANIMATION_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    createDescriptors();
}

void trc::Animation::vulkanStaticDestroy()
{
    auto& device = vkb::getDevice();

    device->freeDescriptorSets(descPool, descSet);
    device->destroyDescriptorSetLayout(descLayout);
    device->destroyDescriptorPool(descPool);
}

void trc::Animation::createDescriptors()
{
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1),
    };
    descPool = vkb::getDevice()->createDescriptorPool(
            { vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes }
    );

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex },
        { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex },
    };
    descLayout = vkb::getDevice()->createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    descSet = std::move(vkb::getDevice()->allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo(descPool, descLayout)
    )[0]);

    // Write set
    vk::DescriptorBufferInfo metaBuffer(*animationMetaDataBuffer, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo animBuffer(*animationBuffer, 0, VK_WHOLE_SIZE);
    std::vector<vk::WriteDescriptorSet> writes = {
        vk::WriteDescriptorSet(descSet, 0, 0, vk::DescriptorType::eStorageBuffer, {}, metaBuffer),
        vk::WriteDescriptorSet(descSet, 1, 0, vk::DescriptorType::eStorageBuffer, {}, animBuffer),
    };
    vkb::getDevice()->updateDescriptorSets(writes, {});

    descProvider.setDescriptorSet(descSet);
    descProvider.setDescriptorSetLayout(descLayout);
}
