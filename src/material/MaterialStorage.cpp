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

auto MaterialStorage::getMaterial(MatID id, PipelineVertexParams params) -> MaterialRuntimeInfo&
{
    assert(id < materialFactories.size());

    return materialFactories.at(id).getOrMake({ .vertexParams=params });
}



MaterialStorage::MaterialFactory::MaterialFactory(MaterialInfo info)
    :
    materialCreateInfo(info)
{
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

}
