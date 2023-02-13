#include "trc/assets/TextureRegistry.h"

#include "texture.pb.h"
#include "trc/assets/import/InternalFormat.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



namespace trc
{

void AssetData<Texture>::serialize(std::ostream& os) const
{
    serial::Texture tex = internal::serializeAssetData(*this);
    tex.SerializeToOstream(&os);
}

void AssetData<Texture>::deserialize(std::istream& is)
{
    serial::Texture tex;
    tex.ParseFromIstream(&is);
    *this = internal::deserializeAssetData(tex);
}



AssetHandle<Texture>::AssetHandle(TextureRegistry::CacheItemRef ref, ui32 deviceIndex)
    :
    cacheRef(std::move(ref)),
    deviceIndex(deviceIndex)
{
}

auto AssetHandle<Texture>::getDeviceIndex() const -> ui32
{
    return deviceIndex;
}



TextureRegistry::TextureRegistry(const TextureRegistryCreateInfo& info)
    :
    device(info.device),
    memoryPool(device, MEMORY_POOL_CHUNK_SIZE),
    dataWriter(info.device),
    descBinding(
        info.descriptorBuilder.addBinding(
            vk::DescriptorType::eCombinedImageSampler,
            MAX_TEXTURE_COUNT,
            vk::ShaderStageFlagBits::eAllGraphics
                | vk::ShaderStageFlagBits::eCompute
                | rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
            vk::DescriptorBindingFlagBits::ePartiallyBound
                | vk::DescriptorBindingFlagBits::eUpdateAfterBind
        )
    )
{
}

void TextureRegistry::update(vk::CommandBuffer cmdBuf, FrameRenderState& frameState)
{
    dataWriter.update(cmdBuf, frameState);
}

auto TextureRegistry::add(u_ptr<AssetSource<Texture>> source) -> LocalID
{
    // Allocate ID
    const LocalID id(idPool.generate());
    const auto deviceIndex = static_cast<LocalID::IndexType>(id);
    if (deviceIndex >= MAX_TEXTURE_COUNT)
    {
        throw std::out_of_range("[In TextureRegistry::add]: Unable to add new texture. Capacity"
                                " of " + std::to_string(id) + " textures has been reached.");
    }

    // Store texture
    std::scoped_lock lock(textureStorageLock);  // Unique ownership
    textures.emplace(
        deviceIndex,
        InternalStorage{
            deviceIndex,
            std::move(source),
            nullptr,
            std::make_unique<ReferenceCounter>(id, this)
        }
    );

    return id;
}

void TextureRegistry::remove(const LocalID id)
{
    std::scoped_lock lock(textureStorageLock);  // Unique ownership

    const LocalID::IndexType index(id);
    textures.erase(index);
    idPool.free(index);
}

auto TextureRegistry::getHandle(const LocalID id) -> Handle
{
    auto& data = textures.get(id);
    return Handle(CacheItemRef(*data.refCounter), data.deviceIndex);
}

void TextureRegistry::load(const LocalID id)
{
    std::scoped_lock lock(textureStorageLock);

    assert(textures.get(id).dataSource != nullptr);
    assert(textures.get(id).deviceData == nullptr);

    auto& tex = textures.get(id);
    auto data = tex.dataSource->load();

    // Create image resource
    Image image(
        device, data.size.x, data.size.y,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        memoryPool.makeAllocator()
    );
    auto imageView = image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm);

    // Write data to device-local image memory
    dataWriter.barrierPreWrite(
        vk::PipelineStageFlagBits::eHost,
        vk::PipelineStageFlagBits::eTransfer,
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eHostWrite,
            vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );
    dataWriter.write(
        *image,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
        { 0, 0, 0 },
        image.getExtent(),
        data.pixels.data(), size_t{data.size.x} * size_t{data.size.y} * 4
    );
    dataWriter.barrierPostWrite(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eComputeShader,
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eMemoryWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            *image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );

    // Create descriptor info
    descBinding.update(
        tex.deviceIndex,
        { image.getDefaultSampler(), *imageView, vk::ImageLayout::eShaderReadOnlyOptimal }
    );

    // Store resources
    tex.deviceData = std::make_unique<InternalStorage::Data>(
        InternalStorage::Data{ std::move(image), std::move(imageView) }
    );
}

void TextureRegistry::unload(LocalID id)
{
    std::scoped_lock lock(textureStorageLock);
    textures.get(id).deviceData.reset();
}

} // namespace trc
