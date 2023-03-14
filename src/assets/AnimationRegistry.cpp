#include "trc/assets/AnimationRegistry.h"

#include "animation.pb.h"
#include "trc/assets/import/InternalFormat.h"



namespace trc
{

void AssetData<Animation>::serialize(std::ostream& os) const
{
    serial::Animation anim = internal::serializeAssetData(*this);
    anim.SerializeToOstream(&os);
}

void AssetData<Animation>::deserialize(std::istream& is)
{
    serial::Animation anim;
    anim.ParseFromIstream(&is);
    *this = internal::deserializeAssetData(anim);
}



AssetHandle<Animation>::AssetHandle(const AnimationData& data, ui32 deviceIndex)
    :
    id(deviceIndex),
    frameCount(data.frameCount),
    durationMs(data.durationMs),
    frameTimeMs(data.frameTimeMs)
{
}

auto AssetHandle<Animation>::getBufferIndex() const noexcept -> ui32
{
    return id;
}

auto AssetHandle<Animation>::getFrameCount() const noexcept -> ui32
{
    return frameCount;
}

auto AssetHandle<Animation>::getDuration() const noexcept -> float
{
    return durationMs;
}

auto AssetHandle<Animation>::getFrameTime() const noexcept -> float
{
    return frameTimeMs;
}

} // namespace trc



trc::AnimationRegistry::AnimationRegistry(const AnimationRegistryCreateInfo& info)
    :
    device(info.device),
    animationMetaDataBuffer(
        info.device,
        MAX_ANIMATIONS * sizeof(AnimationMeta),
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    animationBuffer(
        info.device,
        ANIMATION_BUFFER_SIZE,
        vk::BufferUsageFlagBits::eStorageBuffer
        | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    metaBinding(info.metadataDescBinding),
    dataBinding(info.dataDescBinding)
{
    metaBinding.update(0, { *animationMetaDataBuffer, 0, VK_WHOLE_SIZE });
    dataBinding.update(0, { *animationBuffer, 0, VK_WHOLE_SIZE });
}

void trc::AnimationRegistry::update(vk::CommandBuffer, FrameRenderState&)
{
}

auto trc::AnimationRegistry::add(u_ptr<AssetSource<Animation>> source) -> LocalID
{
    const auto data = source->load();
    const ui32 deviceIndex = makeAnimation(data);
    const LocalID id{ deviceIndex };

    storage.emplace(id, Handle(data, deviceIndex));

    return id;
}

auto trc::AnimationRegistry::getHandle(LocalID id) -> Handle
{
    return storage.get(id);
}

auto trc::AnimationRegistry::makeAnimation(const AnimationData& data) -> ui32
{
    if (data.keyframes.empty())
    {
        throw std::invalid_argument(
            "[In AnimationRegistry::makeAnimation]: Argument of type AnimationData"
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
    assert(data.frameCount == data.keyframes.size());

    auto metaBuf = animationMetaDataBuffer.map<AnimationMeta*>();
    metaBuf[numAnimations] = newMeta;
    animationMetaDataBuffer.unmap();

    // Create new buffer if the current one is too small
    if ((animationBufferOffset + data.frameCount * newMeta.boneCount) * sizeof(mat4)
        > animationBuffer.size())
    {
        Buffer newBuffer(
            device,
            animationBuffer.size() * 2,
            vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        );
        newBuffer.copyFrom(animationBuffer, BufferRegion(0, animationBuffer.size()));

        animationBuffer = std::move(newBuffer);

        dataBinding.update(0, { *animationBuffer, 0, VK_WHOLE_SIZE });
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
