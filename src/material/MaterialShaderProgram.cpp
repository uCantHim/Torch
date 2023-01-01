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
#include "trc/base/Logging.h"
#include "trc/core/PipelineRegistry.h"
#include "trc/material/TorchMaterialSettings.h"
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

using ShaderStageMap = std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>;

auto compileShader(
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

auto compileProgram(
    const ShaderStageMap& stages,
    const std::vector<PipelineLayoutTemplate::Descriptor>& descriptors)
    -> std::unordered_map<vk::ShaderStageFlagBits, std::vector<ui32>>
{
    std::unordered_map<vk::ShaderStageFlagBits, std::vector<ui32>> result;
    for (const auto& [stage, mod] : stages)
    {
        log::info << "Compiling GLSL code for " << vk::to_string(stage) << " stage to SPIRV\n";

        // Set descriptor indices in the shader code
        shader_edit::ShaderDocument doc(mod.getGlslCode());
        for (ui32 i = 0; const auto& desc : descriptors)
        {
            const auto& descName = desc.name.identifier;
            if (auto varName = mod.getDescriptorIndexPlaceholder(descName)) {
                doc.set(*varName, i);
            }
            ++i;
        }

        // Try to compile to SPIRV
        try {
            result.emplace(stage, compileShader(stage, doc.compile()));
        }
        catch (const std::runtime_error& err)
        {
            log::error << "[In makeMaterialProgram]: Unable to compile shader code for stage "
                       << vk::to_string(stage) << " to SPIRV: " << err.what() << "\n";
            log::error << "  >>> Tried to compile the following shader code:\n\n"
                       << mod.getGlslCode()
                       << "\n+++++ END SHADER CODE +++++\n";
        }
    }

    return result;
}

auto collectTextures(const ShaderStageMap& stages)
    -> std::vector<std::pair<ui32, AssetReference<Texture>>>
{
    std::vector<std::pair<ui32, AssetReference<Texture>>> result;
    for (const auto& [stage, mod] : stages)
    {
        for (const auto& texture : mod.getRequiredTextures()) {
            result.emplace_back(texture.specializationConstantIndex, texture.ref.texture);
        }
    }

#ifndef _NDEBUG
    // Ensure that specialization constant indices are unique
    for (auto it = result.begin(); it != result.end(); ++it)
    {
        auto found = std::find_if(it + 1, result.end(),
                                  [idx=it->first](const auto& pair){ return pair.first == idx; });
        assert(found == result.end() && "Specialization constant index is not unique!");
    }
#endif

    return result;
}

auto collectDescriptorSets(const ShaderStageMap& stages, const ShaderDescriptorConfig& descConfig)
    -> std::vector<PipelineLayoutTemplate::Descriptor>
{
    // Collect required descriptor sets
    std::unordered_set<std::string> uniqueDescriptorSets;
    for (const auto& [_, shader] : stages)
    {
        const auto& sets = shader.getRequiredDescriptorSets();
        uniqueDescriptorSets.insert(sets.begin(), sets.end());
    }

    // Assign descriptor sets to their respective indices
    std::vector<PipelineLayoutTemplate::Descriptor> result;
    for (const auto& name : uniqueDescriptorSets)
    {
        const bool isStatic = descConfig.descriptorInfos.at(name).isStatic;
        result.push_back({ DescriptorName{ std::move(name) }, isStatic });
    }

    // Compare descriptor sets based on their preferred index
    auto compDescriptorSetIndex = [&](auto& a, auto& b) -> bool {
        const auto& infos = descConfig.descriptorInfos;
        return infos.at(a.name.identifier).index < infos.at(b.name.identifier).index;
    };
    std::ranges::sort(result, compDescriptorSetIndex);

    return result;
}

// TODO: This is a quick hack. I have to implement much more sophisticated merging
// of push constant ranges for all possible shader stages.
auto collectPushConstants(const ShaderStageMap& stages)
    -> std::vector<MaterialProgramData::PushConstantRange>
{
    using Range = MaterialProgramData::PushConstantRange;

    std::vector<Range> pcRanges;
    for (const auto& [stage, mod] : stages)
    {
        if (mod.getPushConstantSize() <= 0) continue;

        if (stage != vk::ShaderStageFlagBits::eVertex)
        {
            throw std::runtime_error("[In MaterialShaderProgram::makeLayout]: Not implemented:"
                                     " a shader stage other than the vertex stage ("
                                     + vk::to_string(stage) + ") has push constants defined.");
        }

        for (const auto& pc : mod.getPushConstants())
        {
            pcRanges.push_back(Range{
                .offset=pc.offset,
                .size=pc.size,
                .shaderStages=stage,
                .userId=pc.userId,
            });
        }
    }

    return pcRanges;
}

auto makeMaterialProgram(std::unordered_map<vk::ShaderStageFlagBits, ShaderModule> stages)
    -> MaterialProgramData
{
    MaterialProgramData data;

    data.textures = collectTextures(stages);
    data.pushConstants = collectPushConstants(stages);
    data.descriptorSets = collectDescriptorSets(stages, makeShaderDescriptorConfig());

    data.spirvCode = compileProgram(stages, data.descriptorSets);

    return data;
}

void MaterialProgramData::serialize(std::ostream&) const
{
    throw std::logic_error("Not implemented");
}

void MaterialProgramData::deserialize(std::istream&)
{
    throw std::logic_error("Not implemented");
}



MaterialShaderProgram::MaterialShaderProgram(
    const MaterialProgramData& data,
    Pipeline::ID basePipeline)
    :
    layout(makeLayout(data))
{
#ifndef _NDEBUG
    // Debug assertion
    for (const auto& [_, tex] : data.textures)
    {
        assert(tex.hasResolvedID()
               && "Textures must be resolved before being passed into MaterialShaderProgram!");
    }
#endif

    assert(basePipeline != Pipeline::ID::NONE);
    assert(runtimePcOffsets != nullptr);

    // Create shader program
    ProgramDefinitionData program;
    for (const auto& [stage, code] : data.spirvCode) {
        program.stages.emplace(stage, ProgramDefinitionData::ShaderStage{ code });
    }

    // Load textures and set specialization constants
    auto& fragmentStage = program.stages.at(vk::ShaderStageFlagBits::eFragment);
    for (const auto& [specIdx, texRef] : data.textures)
    {
        // Load texture asset
        assert(texRef.hasResolvedID());
        auto& tex = loadedTextures.emplace_back(texRef.getID().getDeviceDataHandle());

        // Set specialization constant
        fragmentStage.specConstants.set(specIdx, tex.getDeviceIndex());
    }
    assert(loadedTextures.size() == data.textures.size());

    // Create pipeline
    const auto base = PipelineRegistry::cloneGraphicsPipeline(basePipeline);
    pipeline = PipelineRegistry::registerPipeline(
        PipelineTemplate{ program, base.getPipelineData() },
        PipelineRegistry::registerPipelineLayout(layout),
        PipelineRegistry::getPipelineRenderPass(basePipeline)
    );

    // Create runtime push constant offsets
    constexpr ui32 alloc = std::numeric_limits<ui32>::max();
    for (auto [offset, size, stages, userId] : data.pushConstants)
    {
        runtimePcOffsets->resize(glm::max(size_t{userId + 1}, runtimePcOffsets->size()), alloc);
        runtimePcOffsets->at(userId) = offset;
    }
}

auto MaterialShaderProgram::getLayout() const -> const PipelineLayoutTemplate&
{
    return layout;
}

auto MaterialShaderProgram::makeRuntime() const -> MaterialRuntime
{
    assert(pipeline != Pipeline::ID::NONE);
    assert(runtimePcOffsets != nullptr);

    return MaterialRuntime(pipeline, runtimePcOffsets);
}

auto MaterialShaderProgram::makeLayout(const MaterialProgramData& data) -> PipelineLayoutTemplate
{
    using Hash = decltype([](vk::ShaderStageFlags flags){ return static_cast<ui32>(flags); });

    std::unordered_map<vk::ShaderStageFlags, vk::PushConstantRange, Hash> perStage;
    for (const auto& range : data.pushConstants)
    {
        auto [it, _] = perStage.try_emplace(range.shaderStages,
                                            vk::PushConstantRange(range.shaderStages, 0, 0));
        vk::PushConstantRange& totalRange = it->second;
        totalRange.size += range.size;
    }

    std::vector<PipelineLayoutTemplate::PushConstant> pushConstants;
    for (const auto& [stage, range] : perStage)
    {
        pushConstants.push_back({
            .range=range,
            .defaultValue=std::nullopt
        });
    }

    PipelineLayoutTemplate layout(data.descriptorSets, std::move(pushConstants));

    return layout;
}

} // namespace trc
