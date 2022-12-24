#include "trc/material/MaterialShaderProgram.h"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include <shader_tools/ShaderDocument.h>
#include <spirv/CompileSpirv.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "trc/VulkanInclude.h"
#include "trc/ShaderLoader.h"
#include "trc/core/PipelineRegistry.h"
#include "trc/util/TorchDirectories.h"



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

namespace trc
{

MaterialShaderProgram::MaterialShaderProgram(
    std::unordered_map<vk::ShaderStageFlagBits, ShaderModule> stages,
    Pipeline::ID basePipeline,
    const ShaderDescriptorConfig& descriptorConfig)
    :
    basePipeline(basePipeline)
{
    // Debug assertion
    for (const auto& [_, mod] : stages)
    {
        for (const auto& tex : mod.getRequiredTextures())
        {
            assert(tex.ref.texture.hasResolvedID()
                   && "Textures must be resolved before being passed into MaterialShaderProgram!");
        }
    }

    layout = makeLayout(stages, descriptorConfig);
    program = makeProgram(stages, layout);

    for (const auto& [stage, shader] : stages)
    {
        util::merge(pushConstantConfig, shader.getPushConstants());

        for (const auto& [textureRef, specIdx] : shader.getRequiredTextures()) {
            specializationTextures.emplace_back(specIdx, textureRef);
        }
    }
}

auto MaterialShaderProgram::getLayout() const -> const PipelineLayoutTemplate&
{
    return layout;
}

auto MaterialShaderProgram::makeRuntime() -> MaterialRuntime
{
    if (pipeline == Pipeline::ID::NONE) {
        initPipeline();
    }
    assert(pipeline != Pipeline::ID::NONE);
    assert(loadedTextures.size() == specializationTextures.size());
    assert(runtimePcOffsets != nullptr);

    return MaterialRuntime(pipeline, runtimePcOffsets);
}

auto MaterialShaderProgram::compileShader(
    vk::ShaderStageFlagBits shaderStage,
    const std::string& glslCode)
    -> std::vector<ui32>
{
    // Set up environment
    auto includer = std::make_unique<spirv::FileIncluder>(
        util::getInternalShaderStorageDirectory(),
        std::vector<fs::path>{ util::getInternalShaderBinaryDirectory() }
    );
    shaderc::CompileOptions opts{ ShaderLoader::makeDefaultOptions() };
    opts.SetIncluder(std::move(includer));

    // Compile source
    const auto result = spirv::generateSpirv(
        glslCode,
        "foo" + shaderStageToExtension(shaderStage),
        opts
    );
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error("[In MaterialShaderProgram::compileShader]: Compile error when"
                                 " compiling source of shader stage " + vk::to_string(shaderStage)
                                 + " to SPIRV: " + result.GetErrorMessage());
    }

    return { result.begin(), result.end() };
}

auto MaterialShaderProgram::makeProgram(
    const ShaderStageMap& stages,
    const PipelineLayoutTemplate& layout) -> ProgramDefinitionData
{
    ProgramDefinitionData program;
    for (const auto& [stage, shaderModule] : stages)
    {
        // Set descriptor indices in the shader code
        shader_edit::ShaderDocument doc(shaderModule.getGlslCode());
        for (ui32 i = 0; const auto& desc : layout.getDescriptors())
        {
            const auto& descName = desc.name.identifier;
            if (auto varName = shaderModule.getDescriptorIndexPlaceholder(descName)) {
                doc.set(*varName, i);
            }
            ++i;
        }

        // Compile GLSL to SPIRV
        auto spirv = compileShader(stage, doc.compile(false));
        program.stages.emplace(stage, ProgramDefinitionData::ShaderStage{ std::move(spirv) });
    }

    return program;
}

auto MaterialShaderProgram::makeLayout(
    const ShaderStageMap& stages,
    const ShaderDescriptorConfig& descConf) -> PipelineLayoutTemplate
{
    using Descriptor = PipelineLayoutTemplate::Descriptor;
    using PushConstant = PipelineLayoutTemplate::PushConstant;

    // Merge descriptor sets and push constants from all stages
    auto [sets, pushConstants] = [&]{
        // Collect required descriptor sets
        std::unordered_set<std::string> uniqueDescriptorSets;
        for (const auto& [_, shader] : stages)
        {
            const auto& sets = shader.getRequiredDescriptorSets();
            uniqueDescriptorSets.insert(sets.begin(), sets.end());
        }

        // Collect push constant ranges
        std::vector<PushConstant> pcRanges;
        for (const auto& [stage, shader] : stages)
        {
            if (shader.getPushConstantSize() > 0)
            {
                if (stage != vk::ShaderStageFlagBits::eVertex)
                {
                    throw std::runtime_error("[In MaterialShaderProgram::makeLayout]: Not implemented:"
                                             " a shader stage other than the vertex stage ("
                                             + vk::to_string(stage) + ") has push constants defined.");
                }
                pcRanges.push_back({
                    vk::PushConstantRange(stage, 0, shader.getPushConstantSize()),
                    {}
                });
            }
        }

        return std::make_pair(uniqueDescriptorSets, pcRanges);
    }();

    // Assign descriptor sets to their respective indices
    std::vector<Descriptor> descriptors;
    for (auto& name : sets)
    {
        const bool isStatic = descConf.descriptorInfos.at(name).isStatic;
        descriptors.push_back({ DescriptorName{ std::move(name) }, isStatic });
    }

    // Compare descriptor sets based on their preferred index
    auto compDescriptorSetIndex = [&](auto& a, auto& b) -> bool {
        const auto& infos = descConf.descriptorInfos;
        return infos.at(a.name.identifier).index < infos.at(b.name.identifier).index;
    };
    std::ranges::sort(descriptors, compDescriptorSetIndex);

    PipelineLayoutTemplate layout(std::move(descriptors), std::move(pushConstants));

    return layout;
}

void MaterialShaderProgram::initPipeline()
{
    assert(pipeline == Pipeline::ID::NONE
        && "Must only call initPipeline if none of the runtime resource have been initialized.");
    assert(loadedTextures.empty()
        && "Must only call initPipeline if none of the runtime resource have been initialized.");
    assert(runtimePcOffsets != nullptr);
    assert(runtimePcOffsets->empty()
        && "Must only call initPipeline if none of the runtime resource have been initialized.");

    constexpr ui32 alloc = std::numeric_limits<ui32>::max();
    for (auto [offset, size, userId] : pushConstantConfig)
    {
        runtimePcOffsets->resize(glm::max(size_t{userId + 1}, runtimePcOffsets->size()), alloc);
        runtimePcOffsets->at(userId) = offset;
    }

    // Load textures and set specialization constants
    auto& fragmentStage = program.stages.at(vk::ShaderStageFlagBits::eFragment);
    for (auto& [specIdx, texRef] : specializationTextures)
    {
        // Load texture asset
        assert(texRef.texture.hasResolvedID());
        auto& tex = loadedTextures.emplace_back(texRef.texture.getID().getDeviceDataHandle());

        // Set specialization constant
        fragmentStage.specConstants.set(specIdx, tex.getDeviceIndex());
    }

    // Create pipeline
    const auto base = PipelineRegistry::cloneGraphicsPipeline(basePipeline);
    pipeline = PipelineRegistry::registerPipeline(
        PipelineTemplate{ program, base.getPipelineData() },
        PipelineRegistry::registerPipelineLayout(layout),
        PipelineRegistry::getPipelineRenderPass(basePipeline)
    );
}

} // namespace trc
