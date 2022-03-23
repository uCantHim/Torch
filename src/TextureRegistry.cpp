#include "TextureRegistry.h"

#include "assets/RawData.h"
#include "ray_tracing/RayPipelineBuilder.h"



trc::TextureRegistry::Handle::Handle(InternalStorage& s)
    :
    SharedCacheReference(s)
{
}

auto trc::TextureRegistry::Handle::getDeviceIndex() const -> ui32
{
    return cache->deviceIndex;
}



trc::TextureRegistry::TextureRegistry(const AssetRegistryModuleCreateInfo& info)
    :
    device(info.device),
    config(info),
    memoryPool(device, MEMORY_POOL_CHUNK_SIZE)
{
}

void trc::TextureRegistry::update(vk::CommandBuffer)
{
    // Nothing
}

auto trc::TextureRegistry::getDescriptorLayoutBindings()
    -> std::vector<DescriptorLayoutBindingInfo>
{
    return {
        {
            config.textureBinding,
            vk::DescriptorType::eCombinedImageSampler,
            MAX_TEXTURE_COUNT,
            vk::ShaderStageFlagBits::eAllGraphics
                | vk::ShaderStageFlagBits::eCompute
                | rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
            {}, vk::DescriptorBindingFlagBits::ePartiallyBound
        }
    };
}

auto trc::TextureRegistry::getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet>
{
    // DescriptorImageInfos need to be kept in memory because they are
    // referenced in the returned WriteDescriptorSets
    std::swap(oldDescriptorUpdates, descriptorUpdates);
    descriptorUpdates.clear();

    std::vector<vk::WriteDescriptorSet> result;
    for (const auto& [index, info] : oldDescriptorUpdates)
    {
        result.emplace_back(
            vk::WriteDescriptorSet(
                {},
                config.textureBinding, index,
                vk::DescriptorType::eCombinedImageSampler,
                info
            )
        );
    }

    return result;
}

auto trc::TextureRegistry::add(u_ptr<AssetSource<Texture>> source) -> LocalID
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
    textures.emplace(
        deviceIndex,
        new InternalStorage{
            CacheRefCounter(id, this),  // Base
            deviceIndex,
            std::move(source),
            nullptr,
        }
    );

    return id;
}

void trc::TextureRegistry::remove(const LocalID id)
{
    const LocalID::IndexType index(id);

    textures.erase(index);
    idPool.free(index);
}

auto trc::TextureRegistry::getHandle(const LocalID id) -> Handle
{
    assert(textures.get(id) != nullptr);

    return Handle(*textures.get(id));
}

void trc::TextureRegistry::load(LocalID id)
{
    assert(textures.get(id) != nullptr);

    auto& tex = *textures.get(id);
    assert(tex.deviceData == nullptr);

    auto data = tex.dataSource->load();
    const ui32 deviceIndex = tex.deviceIndex;

    // Create image resource
    vkb::Image image(
        device, data.size.x, data.size.y,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        memoryPool.makeAllocator()
    );
    image.writeData(data.pixels.data(), data.size.x * data.size.y * 4, {});
    auto imageView = image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm);

    // Create descriptor info
    descriptorUpdates.emplace_back(
        deviceIndex,
        vk::DescriptorImageInfo(
            image.getDefaultSampler(), *imageView,
            vk::ImageLayout::eShaderReadOnlyOptimal
        )
    );

    // Store resources
    tex.deviceData = std::make_unique<InternalStorage::Data>(
        std::move(image),
        std::move(imageView)
    );
}

void trc::TextureRegistry::unload(LocalID id)
{
    assert(textures.get(id) != nullptr);
    assert(textures.get(id)->deviceData != nullptr);

    textures.get(id)->deviceData.reset();
}
