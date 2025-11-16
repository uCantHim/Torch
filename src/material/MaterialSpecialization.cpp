#include "trc/material/MaterialSpecialization.h"

#include "trc/material/ShaderStageInputLinker.h"
#include "trc/material/TorchMaterialSettings.h"
#include "trc/material/VertexShader.h"
#include "trc/material/shader/ShaderModuleCompiler.h"



namespace trc
{

auto makeDeferredMaterialSpecialization(const shader::ShaderModule& fragmentModule,
                                        const MaterialSpecializationInfo& info)
    -> shader::ShaderProgramData
{
    // Create the corresponding vertex shader module.
    VertexModuleCreateInfo vertConfig{ .animated=info.animated };
    VertexModule vertShader{ vertConfig };
    shader::ShaderModuleBuilder vertBuilder{ vertShader.makeCapabilityConfig() };
    auto vertOutputs = vertShader.makeOutputs(vertBuilder);

    // Link shader inputs/outputs across modules.
    ModuleLinkInfo vertexLink{ &vertBuilder, &vertBuilder.getResourceInterface(), &vertOutputs };
    ModuleLinkInfo fragmentLink{ .builder=nullptr, .resources=&fragmentModule, .outputs=nullptr, };
    auto inputLinkRes = linkShaderStageInputs({
        { vk::ShaderStageFlagBits::eVertex, vertexLink },
        { vk::ShaderStageFlagBits::eFragment, fragmentLink },
    });
    if (!inputLinkRes)
    {
        auto& err = inputLinkRes.error();
        throw std::runtime_error("[In makeDeferredMaterialSpecialization] Unable to link shader"
                                 " stage inputs: Shader stage "
                                 + vk::to_string(err.unresolvedInputs.begin()->first)
                                 + " has unresolved capabilities.");
    }

    // Build the vertex module now that all outputs required by the fragment
    // shader have been requested.
    //
    // We can take the fragment module as a fully built module because its
    // outputs are never modified by the shader stage input linker.
    auto vertexModule = shader::ShaderModuleCompiler{}.compile(vertOutputs, vertBuilder);

    // Configure shader program linking.
    auto programLinkSettings = makeProgramLinkerSettings();
    programLinkSettings.inputLocationMapping = inputLinkRes->locationMap;

    // Link the shader stages to a program.
    auto prog = shader::linkShaderProgram(
        {
            { vk::ShaderStageFlagBits::eVertex,   std::move(vertexModule) },
            { vk::ShaderStageFlagBits::eFragment, fragmentModule },
        },
        programLinkSettings
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
