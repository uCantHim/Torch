#include "MaterialRegistry.h"

#include "assets/RawData.h"
#include "ray_tracing/RayPipelineBuilder.h"
#include "TextureRegistry.h"
#include "AssetManager.h"



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
    if (materialBuffer.size() < sizeof(MaterialDeviceData) * materials.size())
    {
        throw std::runtime_error("[In MaterialRegistry::update]: Material buffer is too small!");
    }

    auto buf = materialBuffer.map<MaterialDeviceData*>();
    for (const auto& mat : materials)
    {
        buf[mat.bufferIndex] = mat.matData;
    }
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

auto trc::MaterialRegistry::add(const MaterialData& data) -> LocalID
{
    const LocalID id(idPool.generate());
    const ui32 bufferIndex{ id };

    materials.emplace(
        static_cast<LocalID::IndexType>(id),
        InternalStorage{
            .bufferIndex = bufferIndex,
            .matData = data,
            .albedoTex = data.albedoTexture.hasResolvedID()
                ? data.albedoTexture.getID().getDeviceDataHandle()
                : std::optional<AssetHandle<Texture>>{},
            .normalTex = data.normalTexture.hasResolvedID()
                ? data.normalTexture.getID().getDeviceDataHandle()
                : std::optional<AssetHandle<Texture>>{},
        }
    );

    return id;
}

void trc::MaterialRegistry::remove(LocalID id)
{
    idPool.free(static_cast<LocalID::IndexType>(id));
}

auto trc::MaterialRegistry::getHandle(LocalID id) -> MaterialDeviceHandle
{
    return materials.at(static_cast<LocalID::IndexType>(id));
}



trc::MaterialRegistry::MaterialDeviceData::MaterialDeviceData(const MaterialData& data)
    :
    color(data.color, data.opacity),
    kAmbient(data.ambientKoefficient),
    kDiffuse(data.diffuseKoefficient),
    kSpecular(data.specularKoefficient),
    shininess(data.shininess),
    reflectivity(data.reflectivity),
    diffuseTexture(data.albedoTexture.hasResolvedID()
            ? data.albedoTexture.getID().getDeviceDataHandle().getDeviceIndex()
            : NO_TEXTURE),
    specularTexture(NO_TEXTURE),
    bumpTexture(data.normalTexture.hasResolvedID()
            ? data.normalTexture.getID().getDeviceDataHandle().getDeviceIndex()
            : NO_TEXTURE),
    performLighting(data.doPerformLighting)
{
}
