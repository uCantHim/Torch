#include "MaterialRegistry.h"

#include "ray_tracing/RayPipelineBuilder.h"



trc::MaterialRegistry::MaterialRegistry(const AssetRegistryModuleCreateInfo& info)
    :
    config(info),
    materialBuffer(
        info.device,
        MATERIAL_BUFFER_DEFAULT_SIZE,  // Default material buffer size
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    matBufferDescInfo(*materialBuffer, 0, VK_WHOLE_SIZE)
{
}

void trc::MaterialRegistry::update(vk::CommandBuffer)
{
    if (materialBuffer.size() < sizeof(MaterialDeviceHandle) * materials.size())
    {
        throw std::runtime_error("[In MaterialRegistry::update]: Material buffer is too small!");
    }

    auto buf = materialBuffer.map();
    memcpy(buf, &materials[0], sizeof(MaterialDeviceHandle) * materials.size());
    materialBuffer.unmap();
}

auto trc::MaterialRegistry::getDescriptorLayoutBindings()
    -> std::vector<DescriptorLayoutBindingInfo>
{
    return {
        {
            config.materialBufBinding,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eAllGraphics
                | vk::ShaderStageFlagBits::eCompute
                | rt::ALL_RAY_PIPELINE_STAGE_FLAGS
        }
    };
}

auto trc::MaterialRegistry::getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet>
{
    return {
        vk::WriteDescriptorSet(
            {}, config.materialBufBinding, 0, vk::DescriptorType::eStorageBuffer,
            {},
            matBufferDescInfo
        )
    };
}

auto trc::MaterialRegistry::add(const MaterialDeviceHandle& data) -> LocalID
{
    const LocalID id(idPool.generate());
    materials.emplace(static_cast<LocalID::Type>(id), data);
    return id;
}

void trc::MaterialRegistry::remove(LocalID id)
{
    idPool.free(static_cast<LocalID::Type>(id));
}

auto trc::MaterialRegistry::getHandle(LocalID id) -> MaterialDeviceHandle
{
    return materials.at(static_cast<LocalID::Type>(id));
}
