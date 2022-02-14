#include "AnimationDataStorage.h"

#include "core/Instance.h"
#include "Animation.h"



trc::AnimationDataStorage::AnimationDataStorage(const Instance& instance)
    :
    instance(instance),
    animationMetaDataBuffer(
        instance.getDevice(),
        MAX_ANIMATIONS * sizeof(AnimationMeta),
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    animationBuffer(
        instance.getDevice(),
        ANIMATION_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer
        | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
    createDescriptor(instance);
    writeDescriptor(instance);
}

auto trc::AnimationDataStorage::makeAnimation(const AnimationData& data) -> ui32
{
    if (data.keyframes.empty())
    {
        throw std::invalid_argument(
            "[In AnimationDataStorage::makeAnimation]: Argument of type AnimationData"
            " must have at least one keyframe!"
        );
    }

    std::lock_guard lock(animationCreateLock);

    // Write animation meta data to buffer
    AnimationMeta newMeta{
        .offset = animationBufferOffset,
        .frameCount = data.frameCount,
        .boneCount = static_cast<ui32>(data.keyframes[0].boneMatrices.size())
    };

    auto metaBuf = animationMetaDataBuffer.map(numAnimations * sizeof(AnimationMeta));
    memcpy(metaBuf, &newMeta, sizeof(AnimationMeta));
    animationMetaDataBuffer.unmap();

    // Create new buffer if the current one is too small
    if ((animationBufferOffset + data.frameCount * newMeta.boneCount) * sizeof(mat4)
        > animationBuffer.size())
    {
        vkb::Buffer newBuffer(
            instance.getDevice(),
            animationBuffer.size() * 2,
            vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        );
        newBuffer.copyFrom(animationBuffer, vkb::BufferRegion(0, animationBuffer.size()));
        animationBuffer = std::move(newBuffer);

        writeDescriptor(instance);
    }

    // Copy animation into buffer
    auto animBuf = animationBuffer.map(animationBufferOffset * sizeof(mat4));
    for (size_t offset = 0; const auto& kf : data.keyframes)
    {
        const size_t copySize = kf.boneMatrices.size() * sizeof(mat4);

        memcpy(animBuf + offset, kf.boneMatrices.data(), copySize);
        offset += copySize;
    }
    animationBuffer.unmap();

    numAnimations++;
    animationBufferOffset += data.frameCount * newMeta.boneCount;

    // Create the animation handle
    return numAnimations - 1;
}

auto trc::AnimationDataStorage::getProvider() const -> const DescriptorProviderInterface&
{
    return descProvider;
}

void trc::AnimationDataStorage::createDescriptor(const Instance& instance)
{
    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1),
    };
    descPool = instance.getDevice()->createDescriptorPoolUnique(
            { vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes }
    );

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex },
        { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex },
    };
    descLayout = instance.getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Set
    descSet = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    descProvider.setDescriptorSet(*descSet);
    descProvider.setDescriptorSetLayout(*descLayout);
}

void trc::AnimationDataStorage::writeDescriptor(const Instance& instance)
{
    vk::DescriptorBufferInfo metaBuffer(*animationMetaDataBuffer, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo animBuffer(*animationBuffer, 0, VK_WHOLE_SIZE);
    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 0, 0, vk::DescriptorType::eStorageBuffer, {}, metaBuffer },
        { *descSet, 1, 0, vk::DescriptorType::eStorageBuffer, {}, animBuffer },
    };
    instance.getDevice()->updateDescriptorSets(writes, {});
}
