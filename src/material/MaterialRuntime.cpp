#include "trc/material/MaterialRuntime.h"

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>

#include <shader_tools/ShaderDocument.h>
#include <spirv/CompileSpirv.h>
#include <spirv/FileIncluder.h>

#include "trc/core/PipelineBuilder.h"
#include "trc/drawable/DefaultDrawable.h"



namespace trc
{

auto mergeDescriptorConfigs(const ShaderDescriptorConfig& a, const ShaderDescriptorConfig& b)
    -> ShaderDescriptorConfig
{
    ShaderDescriptorConfig result{ a };

    // Merge descriptor infos
    for (const auto& [name, info] : b.descriptorInfos)
    {
        auto [it, success] = result.descriptorInfos.try_emplace(name, info);
        if (!success && it->second != info)
        {
            throw std::runtime_error("[In mergeRuntimeConfigs]: Descriptor set \"" + name + "\""
                                     " has multiple conflicting definitions.");
        }
    }

    return result;
}

auto shaderStageToExtension(vk::ShaderStageFlagBits stage) -> std::string
{
    switch (stage)
    {
    case vk::ShaderStageFlagBits::eVertex: return ".vert";
    case vk::ShaderStageFlagBits::eGeometry: return ".geom";
    case vk::ShaderStageFlagBits::eTessellationControl: return ".tese";
    case vk::ShaderStageFlagBits::eTessellationEvaluation: return ".tesc";
    case vk::ShaderStageFlagBits::eFragment: return ".frag";

    case vk::ShaderStageFlagBits::eTaskEXT: return ".task";
    case vk::ShaderStageFlagBits::eMeshEXT: return ".mesh";

    case vk::ShaderStageFlagBits::eRaygenKHR: return ".rgen";
    case vk::ShaderStageFlagBits::eIntersectionKHR: return ".rint";
    case vk::ShaderStageFlagBits::eMissKHR: return ".rmiss";
    case vk::ShaderStageFlagBits::eAnyHitKHR: return ".rahit";
    case vk::ShaderStageFlagBits::eClosestHitKHR: return ".rchit";
    case vk::ShaderStageFlagBits::eCallableKHR: return ".rcall";
    default:
        throw std::runtime_error("[In shaderStageToExtension]: Shader stage "
                                 + vk::to_string(stage) + " is not implemented.");
    }

    assert(false);
    throw std::logic_error("");
}



MaterialRuntimeInfo::MaterialRuntimeInfo(
    const ShaderDescriptorConfig& runtimeConf,
    PipelineVertexParams vert,
    PipelineFragmentParams frag,
    std::unordered_map<vk::ShaderStageFlagBits, ShaderModule> stages)
    :
    vertParams(vert),
    fragParams(frag),
    vertResourceHandler(stages.at(vk::ShaderStageFlagBits::eVertex))
{
    {
        // Collect required descriptor sets
        std::unordered_set<std::string> descriptorSets;
        for (const auto& [stage, shader] : stages)
        {
            const auto& sets = shader.getRequiredDescriptorSets();
            descriptorSets.insert(sets.begin(), sets.end());
        }

        // Collect push constant ranges
        std::vector<vk::PushConstantRange> pcRanges;
        for (const auto& [stage, shader] : stages)
        {
            if (shader.getPushConstantSize() > 0) {
                pcRanges.push_back(vk::PushConstantRange(stage, 0, shader.getPushConstantSize()));
            }
        }

        layoutTemplate = makeLayout(descriptorSets, pcRanges, runtimeConf);
    }

    // Post-process shader sources
    for (const auto& [stage, shader] : stages)
    {
        // Set descriptor set indices in the shader source
        shader_edit::ShaderDocument doc(shader.getGlslCode());
        for (const auto& setName : shader.getRequiredDescriptorSets())
        {
            auto varName = shader.getDescriptorIndexPlaceholder(setName);
            assert(varName.has_value());
            doc.set(*varName, runtimeConf.descriptorInfos.at(setName).index);
        }
        auto [it, success] = shaderStages.try_emplace(
            stage,
            ShaderStageInfo{ .glslCode=doc.compile(true) }
        );

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
    const std::vector<vk::PushConstantRange>& pushConstantRanges,
    const ShaderDescriptorConfig& descConf)
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
    for (auto range : pushConstantRanges) {
        pushConstants.push_back({ range, {} });
    }

    PipelineLayoutTemplate layout(descriptors, pushConstants);
    return layout;
}

auto MaterialRuntimeInfo::makePipeline(AssetManager& assetManager) -> Pipeline::ID
{
    ProgramDefinitionData program;

    // Assemble stage information into program
    for (const auto& [stage, data] : shaderStages)
    {
        shaderc::CompileOptions opts;
        auto includer = std::make_unique<spirv::FileIncluder>(
            "shaders/",
            std::vector<fs::path>{ "build/generated-sources/shaders/" }
        );
        opts.SetIncluder(std::move(includer));

        const std::string ext = shaderStageToExtension(stage);
        const auto result = spirv::generateSpirv(data.glslCode, "foo" + ext, opts);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            throw std::runtime_error("[In ShaderLoader::load]: Compile error when compiling shader"
                                     " source " + ext + " to SPIRV: "
                                     + result.GetErrorMessage());
        }
        std::string spirv(
            reinterpret_cast<const char*>(result.begin()),
            static_cast<std::streamsize>(
                (result.end() - result.begin()) * sizeof(decltype(result)::element_type)
            )
        );

        auto [it, _] = program.stages.try_emplace(stage,
                                                  ProgramDefinitionData::ShaderStage{ spirv });
        auto& specs = it->second.specConstants;

        textureHandles.reserve(data.textures.size());
        for (const auto& [specIdx, tex] : data.textures)
        {
            AssetReference<Texture> ref = tex.texture;
            if (!ref.hasResolvedID()) {
                ref.resolve(assetManager);
            }
            auto& deviceHandle = textureHandles.emplace_back(ref.getID().getDeviceDataHandle());
            specs.set(specIdx, deviceHandle.getDeviceIndex());
        }
    }

    // Derive pipeline from a manually defined template
    const auto base = determineDrawablePipeline(DrawablePipelineInfo{
        .animated=vertParams.animated,
        .transparent=fragParams.transparent,
    });
    const auto layout = PipelineRegistry::registerPipelineLayout(layoutTemplate);
    const auto rp = PipelineRegistry::getPipelineRenderPass(base);
    const auto basePipeline = PipelineRegistry::cloneGraphicsPipeline(base);

    PipelineTemplate newPipeline{ program, basePipeline.getPipelineData() };

    return PipelineRegistry::registerPipeline(newPipeline, layout, rp);
}

auto MaterialRuntimeInfo::getPushConstantHandler() const -> const RuntimePushConstantHandler&
{
    return vertResourceHandler;
}

} // namespace trc
