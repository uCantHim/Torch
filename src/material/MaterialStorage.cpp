#include "trc/material/MaterialStorage.h"

#include "trc/drawable/DefaultDrawable.h"
#include "trc/material/VertexShader.h"



namespace trc
{

auto makeMaterialProgramSpecialization(
    ShaderModule fragmentModule,
    const MaterialSpecializationInfo& specialization)
    -> std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>
{
    VertexModule vertexShader(specialization.animated);
    ShaderModule vertexModule = vertexShader.build(fragmentModule);

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

auto MaterialStorage::specialize(MatID id, MaterialSpecializationInfo params) -> MaterialRuntime
{
    if (materialFactories.size() <= id) {
        throw std::out_of_range("[In MaterialStorage::specialize]: No material exists at the"
                                " given ID " + std::to_string(id));
    }

    return materialFactories.at(id).getOrMake({ .vertexParams=params });
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
                                                        specialization.vertexParams);

        Pipeline::ID basePipeline = determineDrawablePipeline(DrawablePipelineInfo{
            .animated=specialization.vertexParams.animated,
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
