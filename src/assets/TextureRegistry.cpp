#include "assets/TextureRegistry.h"

#include "ray_tracing/RayPipelineBuilder.h"



namespace trc
{

AssetHandle<Texture>::AssetHandle(TextureRegistry::CacheItemRef ref)
    :
    cacheRef(std::move(ref))
{
}

auto AssetHandle<Texture>::getDeviceIndex() const -> ui32
{
    return cacheRef->deviceIndex;
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
        new CacheItem<InternalStorage>(
            InternalStorage{
                deviceIndex,
                std::move(source),
                nullptr
            },
            id, this
        )
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
    assert(textures.get(id) != nullptr);

    return Handle(CacheItemRef(*textures.get(id)));
}

void TextureRegistry::load(const LocalID id)
{
    std::scoped_lock lock(textureStorageLock);

    assert(textures.get(id) != nullptr);
    assert(textures.get(id)->getItem().dataSource != nullptr);
    assert(textures.get(id)->getItem().deviceData == nullptr);

    auto& tex = textures.get(id)->getItem();
    auto data = tex.dataSource->load();

    // Create image resource
    vkb::Image image(
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
        data.pixels.data(), data.size.x * data.size.y * 4
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
        std::move(image),
        std::move(imageView)
    );
}

void TextureRegistry::unload(LocalID id)
{
    std::scoped_lock lock(textureStorageLock);
    assert(textures.get(id) != nullptr);

    textures.get(id)->getItem().deviceData.reset();
}

} // namespace trc
