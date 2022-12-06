#include "trc/assets/MaterialRegistry.h"

#include "geometry.pb.h"
#include "trc/assets/AssetManager.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



void trc::AssetData<trc::Material>::serialize(std::ostream&) const
{
    throw std::runtime_error("MaterialData::serialize not implemented!");
}

void trc::AssetData<trc::Material>::deserialize(std::istream&)
{
    throw std::runtime_error("MaterialData::deserialize not implemented!");
}

void trc::AssetData<trc::Material>::resolveReferences(AssetManager&)
{
}



trc::MaterialRegistry::MaterialRegistry(const MaterialRegistryCreateInfo& info)
    :
    materialBuffer(
        info.device,
        std::vector<std::byte>(100, std::byte{0x00}),
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    descBinding(info.descriptorBuilder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eAllGraphics
            | vk::ShaderStageFlagBits::eCompute
            | rt::ALL_RAY_PIPELINE_STAGE_FLAGS
    ))
{
    descBinding.update(0, { *materialBuffer, 0, VK_WHOLE_SIZE });
}

void trc::MaterialRegistry::update(vk::CommandBuffer, FrameRenderState&)
{
}

auto trc::MaterialRegistry::add(u_ptr<AssetSource<Material>> source) -> LocalID
{
    const LocalID id(idPool.generate());

    auto data = source->load();
    if (!data.createInfo.has_value()) {
        return id;
    }

    std::scoped_lock lock(materialStorageLock);
    auto storageId = storage.registerMaterial(*data.createInfo);
    materialIds.emplace(id, storageId);

    return id;
}

void trc::MaterialRegistry::remove(LocalID id)
{
    std::scoped_lock lock(materialStorageLock);
    storage.removeMaterial(materialIds.at(id));
    idPool.free(id);
}

auto trc::MaterialRegistry::getHandle(LocalID id) -> Handle
{
    std::scoped_lock lock(materialStorageLock);
    return Handle{ materialIds.at(id), storage };
}
