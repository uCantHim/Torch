#include "assets/MaterialRegistry.h"

#include "ray_tracing/RayPipelineBuilder.h"



trc::MaterialRegistry::MaterialRegistry(const MaterialRegistryCreateInfo& info)
    :
    materialBuffer(
        info.device,
        MATERIAL_BUFFER_DEFAULT_SIZE,  // Default material buffer size
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
    if (materialBuffer.size() < sizeof(MaterialDeviceData) * materials.size())
    {
        throw std::runtime_error("[In MaterialRegistry::update]: Material buffer is too small!");
    }

    // Execute necessary material updates
    std::scoped_lock lock(changedMaterialsLock);
    if (!changedMaterials.empty())
    {
        std::scoped_lock _lock(materialStorageLock);

        auto buf = materialBuffer.map<MaterialDeviceData*>();
        for (const LocalID id : changedMaterials)
        {
            const auto& mat = materials.at(id);
            buf[mat->bufferIndex] = *mat;
        }
        materialBuffer.unmap();
    }
    changedMaterials.clear();
}

auto trc::MaterialRegistry::add(u_ptr<AssetSource<Material>> source) -> LocalID
{
    const LocalID id(idPool.generate());
    const ui32 bufferIndex{ id };

    auto data = source->load();

    std::scoped_lock lock(materialStorageLock);
    if (materials.size() <= id) {
        materials.resize(id + 1);
    }
    materials.emplace(
        materials.begin() + id,
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

    std::scoped_lock _lock(changedMaterialsLock);
    changedMaterials.emplace(id);

    return id;
}

void trc::MaterialRegistry::remove(LocalID id)
{
    std::scoped_lock lock(materialStorageLock);
    materials.at(id).reset();
    idPool.free(id);
}

auto trc::MaterialRegistry::getHandle(LocalID id) -> Handle
{
    std::scoped_lock lock(materialStorageLock);
    return Handle(materials.at(id)->bufferIndex);
}

auto trc::MaterialRegistry::getData(LocalID id) -> MaterialData
{
    std::scoped_lock lock(materialStorageLock);
    return materials.at(id)->matData;
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
