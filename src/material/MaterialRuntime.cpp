#include "trc/material/MaterialRuntime.h"

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>

#include <shader_tools/ShaderDocument.h>

#include "trc/core/PipelineBuilder.h"
#include "trc/drawable/DefaultDrawable.h"



namespace trc
{

MaterialRuntimeInfo::MaterialRuntimeInfo(
    const DescriptorConfig& descConf,
    PipelineVertexParams vert,
    PipelineFragmentParams frag,
    std::unordered_map<vk::ShaderStageFlagBits, MaterialCompileResult> stages)
    :
    vertParams(vert),
    fragParams(frag)
{
    // Collect required descriptor sets
    std::unordered_set<std::string> descriptorSets;
    for (const auto& [stage, shader] : stages)
    {
        const auto& sets = shader.getRequiredDescriptorSets();
        descriptorSets.insert(sets.begin(), sets.end());
    }

    layoutTemplate = makeLayout(descriptorSets, descConf);

    // Post-process shader sources
    for (const auto& [stage, shader] : stages)
    {
        // Set descriptor set indices in the shader source
        shader_edit::ShaderDocument doc(shader.getShaderGlslCode());
        for (const auto& setName : shader.getRequiredDescriptorSets())
        {
            auto varName = shader.getDescriptorIndexPlaceholder(setName);
            assert(varName.has_value());
            doc.set(*varName, descConf.descriptorInfos.at(setName).index);
        }
        auto [it, success] = shaderStages.try_emplace(stage, doc.compile(true));

        // Store texture references
        assert(success);
        for (const auto& [tex, specIdx] : shader.getRequiredTextures()) {
            it->second.textures.try_emplace(specIdx, tex);
        }
    }
}

auto MaterialRuntimeInfo::getShaderGlslCode(vk::ShaderStageFlagBits stage) const
    -> const std::string&
{
    return shaderStages.at(stage).glslCode;
}

auto MaterialRuntimeInfo::makeLayout(
    const std::unordered_set<std::string>& descriptorSets,
    const DescriptorConfig& descConf)
    -> PipelineLayoutTemplate
{
    /** Compare descriptor sets based on their preferred index */
    auto comp = [&](auto& a, auto& b) -> bool {
        const auto& infos = descConf.descriptorInfos;
        return infos.at(a.name.identifier).index < infos.at(b.name.identifier).index;
    };

    // Assign descriptor sets to their respective indices
    std::vector<PipelineLayoutTemplate::Descriptor> descriptors;
    for (auto& name : descriptorSets)
    {
        const bool isStatic = descConf.descriptorInfos.at(name).isStatic;
        descriptors.push_back({ DescriptorName{ std::move(name) }, isStatic });
    }
    std::ranges::sort(descriptors, comp);

    // Collect push constant ranges
    std::vector<PipelineLayoutTemplate::PushConstant> pushConstants;
    pushConstants.push_back({
        vk::PushConstantRange{ vk::ShaderStageFlagBits::eVertex, 0, },
        {}
    });

    PipelineLayoutTemplate layout(std::move(descriptors), std::move(pushConstants));
    return layout;
}

auto MaterialRuntimeInfo::makePipeline(AssetManager& assetManager) -> Pipeline::ID
{
    ProgramDefinitionData program;

    // Assemble stage information into program
    for (const auto& [stage, data] : shaderStages)
    {
        auto [it, _] = program.stages.try_emplace(stage, data.glslCode);

        for (const auto& [specIdx, tex] : data.textures)
        {
            AssetReference<Texture> ref = tex.texture;
            if (!ref.hasResolvedID()) {
                ref.resolve(assetManager);
            }
            auto& specs = it->second.specConstants;
            specs.set(specIdx, ref.getID().getDeviceDataHandle().getDeviceIndex());
        }
    }

    // Derive pipeline from a manually defined template
    const auto base = determineDrawablePipeline(DrawablePipelineInfo{
        .animated=vertParams.animated,
        .transparent=fragParams.transparent,
    });
    const auto layout = PipelineRegistry::getPipelineLayout(base);
    const auto rp = PipelineRegistry::getPipelineRenderPass(base);
    const auto basePipeline = PipelineRegistry::cloneGraphicsPipeline(base);

    PipelineTemplate newPipeline{ program, basePipeline.getPipelineData() };

    return PipelineRegistry::registerPipeline(newPipeline, layout, rp);
}

} // namespace trc
