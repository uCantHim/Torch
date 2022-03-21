#include "TextureRegistry.h"

#include "assets/RawData.h"
#include "ray_tracing/RayPipelineBuilder.h"



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
    return {
        vk::WriteDescriptorSet(
            {}, config.textureBinding, 0, vk::DescriptorType::eCombinedImageSampler,
            descImageInfos
        )
    };
}

auto trc::TextureRegistry::add(const TextureData& data) -> LocalID
{
    // Allocate ID
    const LocalID id(idPool.generate());
    const auto deviceIndex = static_cast<LocalID::IndexType>(id);
    if (deviceIndex >= MAX_TEXTURE_COUNT)
    {
        throw std::out_of_range("[In TextureRegistry::add]: Unable to add new texture. Capacity"
                                " of " + std::to_string(id) + " textures has been reached.");
    }

    // Create image resource
    vkb::Image image(
        device, data.size.x, data.size.y,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        memoryPool.makeAllocator()
    );
    image.writeData(data.pixels.data(), data.size.x * data.size.y * 4, {});
    auto view = image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm);

    // Store texture
    auto& tex = textures.emplace(
        deviceIndex,
        InternalStorage{
            .deviceIndex = deviceIndex,
            .image = std::move(image),
            .imageView = std::move(view),
        }
    );

    // Store descriptor info
    descImageInfos.emplace(
        deviceIndex,
        vk::DescriptorImageInfo(
            tex.image.getDefaultSampler(), *tex.imageView,
            vk::ImageLayout::eShaderReadOnlyOptimal
        )
    );

    return id;
}

void trc::TextureRegistry::remove(const LocalID id)
{
    const LocalID::IndexType index(id);

    textures.erase(index);
    descImageInfos.erase(index);
    idPool.free(index);
}

auto trc::TextureRegistry::getHandle(const LocalID id) -> TextureDeviceHandle
{
    return textures.get(static_cast<LocalID::IndexType>(id));
}
