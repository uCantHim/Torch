#include "trc/assets/MaterialRegistry.h"

#include "geometry.pb.h"
#include "trc/assets/AssetManager.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"
#include "trc/drawable/DefaultDrawable.h"



trc::AssetData<trc::Material>::AssetData(ShaderModule fragModule, bool transparent)
    :
    transparent(transparent)
{
    auto specialize = [this, &fragModule](const MaterialSpecializationInfo& info)
    {
        programs[MaterialKey{ info }]
            = makeMaterialProgram(makeMaterialSpecialization(fragModule, info));
    };

    specialize(MaterialSpecializationInfo{ .animated=false });
    specialize(MaterialSpecializationInfo{ .animated=true });
}

void trc::AssetData<trc::Material>::serialize(std::ostream&) const
{
    throw std::runtime_error("MaterialData::serialize not implemented!");
}

void trc::AssetData<trc::Material>::deserialize(std::istream&)
{
    throw std::runtime_error("MaterialData::deserialize not implemented!");
}

void trc::AssetData<trc::Material>::resolveReferences(AssetManager& assetManager)
{
    for (auto& [_, program] : programs)
    {
        for (auto& [_, ref] : program.textures) {
            ref.resolve(assetManager);
        }
    }
}



trc::MaterialRegistry::MaterialRegistry(const MaterialRegistryCreateInfo& info)
    :
    descriptorConfig(info.descriptorConfig),
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
    std::scoped_lock lock(materialStorageLock);
    const LocalID id{ localIdPool.generate() };
    auto& mat = *storage.emplace(id, new Storage{
        .data=source->load(),
        .runtimePrograms={ nullptr }
    });

    // Create all runtime programs
    for (const auto& [key, program] : mat.data.programs)
    {
        Pipeline::ID basePipeline = determineDrawablePipeline(DrawablePipelineInfo{
            .animated=key.flags.has(MaterialKey::Flags::Animated::eTrue),
            .transparent=mat.data.transparent
        });
        mat.runtimePrograms.at(key.flags.toIndex())
            = std::make_unique<MaterialShaderProgram>(program, basePipeline);

        auto rt = mat.runtimePrograms.at(key.flags.toIndex())->makeRuntime();
        auto _rt = mat.getSpecialization(key);
        assert(rt.getPipeline() != Pipeline::ID::NONE);
    }

    return id;
}

void trc::MaterialRegistry::remove(LocalID id)
{
    std::scoped_lock lock(materialStorageLock);
    assert(storage.size() > id);

    storage[id] = {};
    localIdPool.free(id);
}

auto trc::MaterialRegistry::getHandle(LocalID id) -> Handle
{
    std::scoped_lock lock(materialStorageLock);
    assert(storage.size() > id);

    return Handle{ *storage.at(id) };
}
