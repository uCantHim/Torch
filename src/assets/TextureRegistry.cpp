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



TextureRegistry::TextureRegistry(const TextureRegistryCreateInfo& info)
    :
    device(info.device),
    memoryPool(info.device, MEMORY_POOL_CHUNK_SIZE),
    dataWriter(info.device),
    deviceDataStorage(DataCache::makeLoader(
        [this](ui32 id){ return loadDeviceData(LocalID{ id }); },
        [this](ui32 id, DeviceData data){ freeDeviceData(LocalID{ id }, std::move(data)); }
    )),
    descBinding(info.textureDescBinding)
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
    std::scoped_lock lock(sourceStorageLock);  // Unique ownership
    dataSources.emplace(id, std::move(source));

    return id;
}

void TextureRegistry::remove(const LocalID id)
{
    std::scoped_lock lock(sourceStorageLock);  // Unique ownership
    dataSources.erase(id);
    idPool.free(id);
}

auto TextureRegistry::getHandle(const LocalID id) -> Handle
{
    return Handle{ deviceDataStorage.get(id) };
}

auto TextureRegistry::loadDeviceData(const LocalID id) -> DeviceData
{
    std::shared_lock lock(sourceStorageLock);  // Shared ownership as we only read here

    assert(dataSources.contains(id));
    assert(dataSources.get(id) != nullptr);

    const ui32 deviceIndex = ui32{id};
    const auto data = dataSources.get(id)->load();

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
        deviceIndex,
        { image.getDefaultSampler(), *imageView, vk::ImageLayout::eShaderReadOnlyOptimal }
    );

    // Store resources
    return DeviceData{
        .deviceIndex = deviceIndex,
        .image       = std::move(image),
        .imageView   = std::move(imageView)
    };
}

void TextureRegistry::freeDeviceData(const LocalID /*id*/, DeviceData /*data*/)
{
}



AssetHandle<Texture>::AssetHandle(DataHandle handle)
    :
    cacheRef(handle),
    deviceIndex(handle->deviceIndex)
{
}

auto AssetHandle<Texture>::getDeviceIndex() const -> ui32
{
    return deviceIndex;
}

} // namespace trc
