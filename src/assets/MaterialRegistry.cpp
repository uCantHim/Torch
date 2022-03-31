#include "assets/MaterialRegistry.h"

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
    descBinding(info.layoutBuilder->addBinding(
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eAllGraphics
            | vk::ShaderStageFlagBits::eCompute
            | rt::ALL_RAY_PIPELINE_STAGE_FLAGS
    ))
{
    descBinding.update(0, { *materialBuffer, 0, VK_WHOLE_SIZE });
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
        buf[mat->bufferIndex] = *mat;
    }
    materialBuffer.unmap();
}

auto trc::MaterialRegistry::add(u_ptr<AssetSource<Material>> source) -> LocalID
{
    const LocalID id(idPool.generate());
    const ui32 bufferIndex{ id };

    auto data = source->load();

    materials.emplace(
        static_cast<LocalID::IndexType>(id),
        new InternalStorage{
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

auto trc::MaterialRegistry::getHandle(LocalID id) -> Handle
{
    return Handle(materials.at(id)->bufferIndex);
}



trc::MaterialRegistry::MaterialDeviceData::MaterialDeviceData(const InternalStorage& data)
    :
    color(data.matData.color, data.matData.opacity),
    kAmbient(data.matData.ambientKoefficient),
    kDiffuse(data.matData.diffuseKoefficient),
    kSpecular(data.matData.specularKoefficient),
    shininess(data.matData.shininess),
    reflectivity(data.matData.reflectivity),
    diffuseTexture(data.albedoTex.has_value()
            ? data.albedoTex->getDeviceIndex()
            : NO_TEXTURE),
    specularTexture(NO_TEXTURE),
    bumpTexture(data.normalTex.has_value()
            ? data.normalTex->getDeviceIndex()
            : NO_TEXTURE),
    performLighting(data.matData.doPerformLighting)
{
}
