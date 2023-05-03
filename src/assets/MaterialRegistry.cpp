#include "trc/assets/MaterialRegistry.h"

#include "material.pb.h"
#include "trc/DrawablePipelines.h"
#include "trc/assets/AssetManager.h"
#include "trc/drawable/DefaultDrawable.h"
#include "trc/material/TorchMaterialSettings.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



trc::AssetData<trc::Material>::AssetData(ShaderModule fragModule, bool transparent)
    :
    transparent(transparent)
{
    auto specialize = [this, &fragModule](const MaterialSpecializationInfo& info)
    {
        programs[MaterialKey{ info }] = makeMaterialProgram(
            makeMaterialSpecialization(fragModule, info),
            makeShaderDescriptorConfig()
        );
    };

    specialize(MaterialSpecializationInfo{ .animated=false });
    specialize(MaterialSpecializationInfo{ .animated=true });
}

void trc::AssetData<trc::Material>::serialize(std::ostream& os) const
{
    serial::Material mat;
    mat.mutable_settings()->set_transparent(transparent);
    for (const auto& [key, program] : programs)
    {
        auto newSpec = mat.add_specializations();
        *newSpec->mutable_shader_program() = program.serialize();
        newSpec->set_animated(key.flags.has(MaterialKey::Flags::Animated::eTrue));
    }

    mat.SerializeToOstream(&os);
}

void trc::AssetData<trc::Material>::deserialize(std::istream& is)
{
    serial::Material mat;
    mat.ParseFromIstream(&is);

    // Parse settings
    transparent = mat.settings().transparent();

    // Parse specializations
    programs.clear();
    for (const auto& spec : mat.specializations())
    {
        MaterialProgramData program;
        program.deserialize(spec.shader_program());
        programs.try_emplace(
            MaterialSpecializationInfo{ .animated=spec.animated() },
            std::move(program)
        );
    }
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



void trc::MaterialRegistry::update(vk::CommandBuffer, FrameRenderState&)
{
}

auto trc::MaterialRegistry::add(u_ptr<AssetSource<Material>> source) -> LocalID
{
    const LocalID id{ localIdPool.generate() };
    auto& mat = storage.emplace(id, Storage{
        .data=source->load(),
        .runtimePrograms={ nullptr }
    });

    // Create all runtime programs
    for (const auto& [key, program] : mat.data.programs)
    {
        const DrawablePipelineInfo info{
            .animated=key.flags.has(MaterialKey::Flags::Animated::eTrue),
            .transparent=mat.data.transparent
        };
        Pipeline::ID basePipeline = pipelines::getDrawableBasePipeline(info.toPipelineFlags());

        mat.runtimePrograms.at(key.flags.toIndex()) = std::make_unique<MaterialShaderProgram>(
            program,
            basePipeline
        );

        auto rt = mat.runtimePrograms.at(key.flags.toIndex())->makeRuntime();
        auto _rt = mat.getSpecialization(key);
        assert(rt.getPipeline() != Pipeline::ID::NONE);
    }

    return id;
}

void trc::MaterialRegistry::remove(LocalID id)
{
    assert(storage.contains(id));

    storage.erase(id);
    localIdPool.free(id);
}

auto trc::MaterialRegistry::getHandle(LocalID id) -> Handle
{
    assert(storage.contains(id));

    return Handle{ storage.at(id) };
}
