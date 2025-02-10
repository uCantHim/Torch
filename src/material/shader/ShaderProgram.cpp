#include "trc/material/shader/ShaderProgram.h"

#include <algorithm>
#include <ranges>
#include <string>
#include <unordered_set>

#include <shader_tools/ShaderDocument.h>
#include <trc_util/Padding.h>
#include <trc_util/StringManip.h>
#include <trc_util/Timer.h>
#include <trc_util/algorithm/VectorTransform.h>

#include "material_shader_program.pb.h"
#include "trc/base/Logging.h"



class NullIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    NullIncluder(const NullIncluder&) = delete;
    NullIncluder(NullIncluder&&) noexcept = delete;
    NullIncluder& operator=(const NullIncluder&) = delete;
    NullIncluder& operator=(NullIncluder&&) noexcept = delete;
    ~NullIncluder() noexcept override = default;

    NullIncluder() = default;

    /** Handles shaderc_include_resolver_fn callbacks. */
    auto GetInclude(const char* /*requested_source*/,
                    shaderc_include_type /*type*/,
                    const char* /*requesting_source*/,
                    size_t)
        -> shaderc_include_result* override
    {
        return &nullResult;
    }

    /** Handles shaderc_include_result_releultase_fn callbacks. */
    void ReleaseInclude(shaderc_include_result* /*data*/) override
    {
    }

    shaderc_include_result nullResult{
        .source_name = "",
        .source_name_length = 0,
        .content = "No include paths given",
        .content_length = strlen("No include paths given"),
        .user_data=nullptr,
    };
};

namespace trc::shader
{

using ShaderStageMap = std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>;

/**
 * @return Final GLSL shader code for each stage, or an error message.
 */
auto compileProgramCode(
    const ShaderStageMap& stages,
    const std::vector<ShaderProgramData::DescriptorSet>& descriptors,
    const std::vector<ShaderProgramData::PushConstantRange>& pushConstants)
    -> std::expected<
        std::unordered_map<vk::ShaderStageFlagBits, std::string>,
        std::string
    >
{
    std::unordered_map<vk::ShaderStageFlagBits, std::string> result;
    for (const auto& [stage, mod] : stages)
    {
        Timer timer;
        shader_edit::ShaderDocument doc(mod.getShaderCode());

        // Set descriptor indices in the shader code
        for (const auto& desc : descriptors)
        {
            if (auto varName = mod.getDescriptorIndexPlaceholder(desc.name)) {
                doc.set(*varName, desc.index);
            }
        }

        // Set push constant offsets in the shader code
        for (const auto& pc : pushConstants)
        {
            if (auto varName = mod.getPushConstantOffsetPlaceholder(pc.userId)) {
                doc.set(*varName, pc.offset);
            }
        }

        // Try to finalize GLSL
        try {
            result.emplace(stage, doc.compile());
            log::info << "Finalized GLSL code for " << vk::to_string(stage) << " stage"
                      << " in " << timer.reset() << "ms.";
        }
        catch (const shader_edit::CompileError& err)
        {
            log::error << "[In linkMaterialProgram]: Unable to finalize shader code for"
                       << " SPIRV conversion (shader stage " << vk::to_string(stage) << ")"
                       << ": " << err.what();
            return std::unexpected(err.what());
        }
    }

    return result;
}

auto collectDescriptorSets(
    const ShaderStageMap& stages,
    const ShaderProgramLinkSettings& config)
    -> std::vector<ShaderProgramData::DescriptorSet>
{
    // Collect required descriptor sets
    std::unordered_set<std::string> requiredDescriptorSets;
    for (const auto& [_, shader] : stages)
    {
        const auto& sets = shader.getRequiredDescriptorSets();
        requiredDescriptorSets.insert(sets.begin(), sets.end());
    }

    /** Establish an order among DescriptorSets based on preference. */
    auto comp = [&config](const std::string& a, const std::string& b)
    {
        const auto& pref = config.preferredDescriptorSetIndices;
        const auto _a = pref.find(a);
        const auto _b = pref.find(b);

        if (_a != pref.end() && _b != pref.end()) {
            return _a->second < _b->second;
        }
        if (_a != pref.end()) {
            return true;   // A has a priority defined; prioritize it over B
        }
        if (_b != pref.end()) {
            return false;  // B has a priority defined; prioritize it over A
        }
        return a < b;  // Fallback to string comparison
    };

    // Sort descriptor sets by preference
    auto result = std::ranges::to<std::vector>(requiredDescriptorSets);
    std::ranges::sort(result, comp);

    // Generate DescriptorSet structs.
    return result
        | std::views::enumerate
        | std::views::transform([](auto&& pair) {
            auto [i, desc] = pair;
            return ShaderProgramData::DescriptorSet{ .name=desc, .index=static_cast<ui32>(i) };
        })
        | std::ranges::to<std::vector>();
}

// TODO: This is a quick hack. I have to implement much more sophisticated merging
// of push constant ranges for all possible shader stages.
auto collectPushConstants(const ShaderStageMap& stages)
    -> std::vector<ShaderProgramData::PushConstantRange>
{
    using Range = ShaderProgramData::PushConstantRange;

    std::vector<Range> pcRanges;
    ui32 totalOffset{ 0 };
    for (const auto& [stage, mod] : stages)
    {
        if (mod.getPushConstantSize() <= 0) continue;

        for (const auto& pc : mod.getPushConstants())
        {
            pcRanges.push_back(Range{
                .offset=pc.offset + totalOffset,
                .size=pc.size,
                .shaderStage=stage,
                .userId=pc.userId,
            });
        }

        /**
         * The push constants of each stage have their offsets specified with
         * respect to all preceding ranges of the same stage. To each range of
         * subsequent stages, add the total offset of all previous stages.
         *
         * The 16-byte padding is, strictly speaking, slightly over-secure.
         * Theoretically, each member must be offset by a multiple of its own
         * alignment. However, I don't want to figure out the the first member
         * in the next push constant range and deduce its alignment from its
         * type right now. 16 bytes are the largest possible alignment (e.g. of
         * 4x4 matrices) and it always works.
         */
        totalOffset += util::pad_16(mod.getPushConstantSize());
    }

    return pcRanges;
}

auto combinePushConstantsPerStage(
    const std::vector<ShaderProgramData::PushConstantRange>& pushConstants)
    -> std::unordered_map<vk::ShaderStageFlagBits, vk::PushConstantRange>
{
    // Combine push constant ranges into a single one for each shader stage
    std::unordered_map<vk::ShaderStageFlagBits, vk::PushConstantRange> perStage;
    for (const auto& range : pushConstants)
    {
        constexpr ui32 _off = std::numeric_limits<ui32>::max();
        auto [it, _] = perStage.try_emplace(range.shaderStage,
                                            vk::PushConstantRange(range.shaderStage, _off, 0));
        vk::PushConstantRange& totalRange = it->second;
        totalRange.size += range.size;
        totalRange.offset = std::min(totalRange.offset, range.offset);
    }

    return perStage;
}

auto linkShaderProgram(
    std::unordered_map<vk::ShaderStageFlagBits, ShaderModule> stages,
    const ShaderProgramLinkSettings& config)
    -> std::expected<ShaderProgramData, ShaderProgramLinkError>
{
    ShaderProgramData data;

    // Collect specialization constants
    for (const auto& [stage, mod] : stages)
    {
        auto& specs = data.specConstants.try_emplace(stage).first->second;
        for (const auto& spec : mod.getSpecializationConstants()) {
            specs.emplace_back(spec.specializationConstantIndex, spec.value);
        }
    }
    data.pushConstants = collectPushConstants(stages);
    data.pcRangesPerStage = combinePushConstantsPerStage(data.pushConstants);
    data.descriptorSets = collectDescriptorSets(stages, config);

    // Compile each shader module to SPIR-V
    if (auto prog = compileProgramCode(stages, data.descriptorSets, data.pushConstants)) {
        data.glslCode = *prog;
    }
    else {
        return std::unexpected(ShaderProgramLinkError::eShaderCodeFinalizeError);
    }

    return data;
}



auto ShaderProgramData::serialize() const -> serial::ShaderProgram
{
    serial::ShaderProgram prog;
    for (const auto& [stage, mod] : glslCode)
    {
        auto newModule = prog.add_shader_modules();
        newModule->set_spirv_code(mod.data(), mod.size() * sizeof(ui32));
        newModule->set_stage(static_cast<serial::ShaderStageBit>(stage));

        if (specConstants.contains(stage))
        {
            for (const auto& [idx, val] : specConstants.at(stage)) {
                newModule->mutable_specialization_constants()->emplace(idx, val->serialize());
            }
        }
    }

    for (const auto& range : pushConstants)
    {
        auto pc = prog.add_push_constants();
        pc->set_offset(range.offset);
        pc->set_size(range.size);
        pc->set_shader_stage_flags(static_cast<ui32>(range.shaderStage));
        pc->set_user_id(range.userId);
    }

    for (const auto& desc : descriptorSets)
    {
        auto newSet = prog.add_descriptor_sets();
        newSet->set_name(desc.name);
        newSet->set_index(desc.index);
    }

    return prog;
}

void ShaderProgramData::deserialize(
    const serial::ShaderProgram& prog,
    ShaderRuntimeConstantDeserializer& deserializer)
{
    *this = {};  // Clear all data

    for (const auto& mod : prog.shader_modules())
    {
        auto [it, _] = glslCode.try_emplace(static_cast<vk::ShaderStageFlagBits>(mod.stage()));
        auto& code = it->second;

        assert(mod.spirv_code().size() % sizeof(ui32) == 0);
        code.resize(mod.spirv_code().size() / sizeof(ui32));
        memcpy(code.data(), mod.spirv_code().data(), mod.spirv_code().size());

        if (!mod.specialization_constants().empty())
        {
            auto& specs = specConstants.try_emplace(it->first).first->second;
            for (const auto& [idx, value] : mod.specialization_constants())
            {
                if (auto runtimeConst = deserializer.deserialize(value)) {
                    specs.emplace_back(idx, runtimeConst.value());
                }
                else {
                    log::warn << log::here()
                        << ": Deserialization of shader runtime value at specialization constant"
                        << " index " << idx << " failed: deserializer returned std::nullopt.";
                }
            }
        }
    }

    for (const auto& range : prog.push_constants())
    {
        pushConstants.push_back({
            .offset=range.offset(),
            .size=range.size(),
            .shaderStage=vk::ShaderStageFlagBits(range.shader_stage_flags()),
            .userId=range.user_id()
        });
    }
    pcRangesPerStage = combinePushConstantsPerStage(pushConstants);

    for (const auto& desc : prog.descriptor_sets())
    {
        descriptorSets.push_back(ShaderProgramData::DescriptorSet{
            .name=desc.name(),
            .index=desc.index(),
        });
    }
}

void ShaderProgramData::serialize(std::ostream& os) const
{
    serialize().SerializeToOstream(&os);
}

void ShaderProgramData::deserialize(
    std::istream& is,
    ShaderRuntimeConstantDeserializer& deserializer)
{
    serial::ShaderProgram prog;
    prog.ParseFromIstream(&is);
    deserialize(prog, deserializer);
}

} // namespace trc::shader
