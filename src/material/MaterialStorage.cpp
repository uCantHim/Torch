#include "trc/material/MaterialStorage.h"

#include "trc/material/VertexShader.h"



namespace trc
{

auto MaterialStorage::registerMaterial(MaterialInfo info) -> MatID
{
    MatID id = materialFactories.size();
    materialFactories.emplace_back(std::move(info));
    return id;
}

void MaterialStorage::removeMaterial(MatID id)
{
    assert(id < materialFactories.size());
    materialFactories.at(id).clear();
}

auto MaterialStorage::getFragmentParams(MatID id) const -> const PipelineFragmentParams&
{
    return materialFactories.at(id).getInfo().fragmentInfo;
}

auto MaterialStorage::getRuntime(MatID id, PipelineVertexParams params) -> MaterialRuntimeInfo&
{
    assert(id < materialFactories.size());
    return materialFactories.at(id).getOrMake({ .vertexParams=params });
}



MaterialStorage::MaterialFactory::MaterialFactory(MaterialInfo info)
    :
    materialCreateInfo(info)
{
}

auto MaterialStorage::MaterialFactory::getInfo() const -> const MaterialInfo&
{
    return materialCreateInfo;
}

auto MaterialStorage::MaterialFactory::getOrMake(MaterialKey specialization) -> MaterialRuntimeInfo&
{
    auto [it, success] = runtimes.try_emplace(specialization, nullptr);
    if (success)
    {
        VertexModule vertModuleBuilder(specialization.vertexParams.animated);
        auto vertModule = vertModuleBuilder.build(materialCreateInfo.fragmentModule);
        auto fragModule = materialCreateInfo.fragmentModule;

        it->second.reset(new MaterialRuntimeInfo(
            materialCreateInfo.descriptorConfig,
            specialization.vertexParams,
            materialCreateInfo.fragmentInfo,
            {
                { vk::ShaderStageFlagBits::eVertex, std::move(vertModule) },
                { vk::ShaderStageFlagBits::eFragment, std::move(fragModule) },
            }
        ));
    }

    return *it->second;
}

void MaterialStorage::MaterialFactory::clear()
{
    runtimes.clear();
}

}
