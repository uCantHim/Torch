#include "trc/material/MaterialStorage.h"

#include "trc/drawable/DefaultDrawable.h"
#include "trc/material/VertexShader.h"



namespace trc
{

MaterialKey::MaterialKey(MaterialSpecializationInfo info)
{
    if (info.animated) {
        flags |= Flags::Animated::eTrue;
    }
}

MaterialKey::MaterialKey(MaterialSpecializationFlags flags)
    : flags(flags)
{}

bool MaterialKey::operator==(const MaterialKey& rhs) const
{
    return flags.toIndex() == rhs.flags.toIndex();
}



auto makeMaterialProgramSpecialization(
    ShaderModule fragmentModule,
    const MaterialKey& specialization)
    -> std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>
{
    const bool animated = specialization.flags.has(MaterialKey::Flags::Animated::eTrue);
    ShaderModule vertexModule = VertexModule{ animated }.build(fragmentModule);

    return {
        { vk::ShaderStageFlagBits::eVertex,   std::move(vertexModule) },
        { vk::ShaderStageFlagBits::eFragment, std::move(fragmentModule) },
    };
}



MaterialStorage::MaterialStorage(const ShaderDescriptorConfig& descriptorConfig)
    :
    descriptorConfig(descriptorConfig)
{
}

auto MaterialStorage::registerMaterial(MaterialBaseInfo info) -> MatID
{
    MatID id = materialFactories.size();
    materialFactories.emplace_back(this, std::move(info));
    return id;
}

void MaterialStorage::removeMaterial(MatID id)
{
    assert(id < materialFactories.size());
    materialFactories.at(id).clear();
}

auto MaterialStorage::getBaseMaterial(MatID id) const -> const MaterialBaseInfo&
{
    if (materialFactories.size() <= id) {
        throw std::out_of_range("[In MaterialStorage::getBaseMaterial]: No material exists at the"
                                " given ID " + std::to_string(id));
    }

    return materialFactories.at(id).getBase();
}

auto MaterialStorage::specialize(MatID id, MaterialKey key) -> MaterialRuntime
{
    if (materialFactories.size() <= id) {
        throw std::out_of_range("[In MaterialStorage::specialize]: No material exists at the"
                                " given ID " + std::to_string(id));
    }

    return materialFactories.at(id).getOrMake(key);
}



MaterialStorage::MaterialSpecializer::MaterialSpecializer(
    const MaterialStorage* storage,
    MaterialBaseInfo info)
    :
    storage(storage),
    baseMaterial(info)
{
}

auto MaterialStorage::MaterialSpecializer::getBase() const -> const MaterialBaseInfo&
{
    return baseMaterial;
}

auto MaterialStorage::MaterialSpecializer::getOrMake(MaterialKey specialization) -> MaterialRuntime
{
    auto [it, success] = specializations.try_emplace(specialization, nullptr);
    if (success)
    {
        auto stages = makeMaterialProgramSpecialization(baseMaterial.fragmentModule,
                                                        specialization.flags);

        Pipeline::ID basePipeline = determineDrawablePipeline(DrawablePipelineInfo{
            .animated=specialization.flags & MaterialKey::Flags::Animated::eTrue,
            .transparent=baseMaterial.transparent,
        });
        it->second = std::make_unique<MaterialShaderProgram>(
            std::move(stages),
            basePipeline,
            storage->descriptorConfig
        );
    }

    assert(it->second != nullptr);
    return it->second->makeRuntime();
}

void MaterialStorage::MaterialSpecializer::clear()
{
    specializations.clear();
}

}
