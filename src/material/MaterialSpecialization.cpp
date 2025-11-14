#include "trc/material/MaterialSpecialization.h"

#include "trc/material/TorchMaterialSettings.h"
#include "trc/material/VertexShader.h"



namespace trc
{

auto makeDeferredMaterialSpecialization(const shader::ShaderModule& fragmentModule,
                                        const MaterialSpecializationInfo& info)
    -> shader::ShaderProgramData
{
    auto vertexModule = VertexModule{ {.animated=info.animated} }.build(fragmentModule);
    auto prog = shader::linkShaderProgram(
        {
            { vk::ShaderStageFlagBits::eVertex,   std::move(vertexModule) },
            { vk::ShaderStageFlagBits::eFragment, fragmentModule },
        },
        makeProgramLinkerSettings()
    );

    if (!prog) {
        throw std::runtime_error("Unexpected error: Unable to create material specialization."
                                 " A program link error occurred; should this even be possible?");
    }

    return *prog;
}

auto makeDeferredMaterialSpecialization(const MaterialBaseInfo& baseInfo,
                                        const MaterialSpecializationInfo& info)
    -> shader::ShaderProgramData
{
    return makeDeferredMaterialSpecialization(baseInfo.fragmentModule, info);
}



MaterialSpecializationCache::MaterialSpecializationCache(const MaterialBaseInfo& base)
    :
    base(base)
{
}

auto MaterialSpecializationCache::getBaseInfo() const -> const MaterialBaseInfo&
{
    return base;
}

auto MaterialSpecializationCache::getSpecialization(const MaterialKey& key)
    -> const shader::ShaderProgramData&
{
    return getOrCreateSpecialization(key);
}

void MaterialSpecializationCache::createAllSpecializations()
{
    for (auto&& _ : iterSpecializations()) {}
}

auto MaterialSpecializationCache::iterSpecializations()
    -> std::generator<std::pair<MaterialKey, const shader::ShaderProgramData&>>
{
    for (const auto& [i, prog] : std::views::enumerate(shaderPrograms))
    {
        const auto key = MaterialKey::fromUniqueIndex(i);
        co_yield { key, getOrCreateSpecialization(key) };
    }
}

auto MaterialSpecializationCache::getOrCreateSpecialization(const MaterialKey& key)
    -> shader::ShaderProgramData&
{
    auto& program = shaderPrograms[key.toUniqueIndex()];
    if (!program) {
        program = createSpecialization(base, key);
    }

    return program.value();
}

auto MaterialSpecializationCache::createSpecialization(
    const MaterialBaseInfo& base,
    const MaterialKey& key)
    -> shader::ShaderProgramData
{
    return makeDeferredMaterialSpecialization(base.fragmentModule, key.toSpecializationInfo());
}

} // namespace trc
