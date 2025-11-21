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

#include "trc/serial/material_shader_program.pb.h"
#include "trc/base/Logging.h"



namespace trc::shader
{

using ShaderStageMap = std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>;

/**
 * @return Final GLSL shader code for each stage, or an error message.
 */
auto compileProgramCode(
    const ShaderStageMap& stages,
    const std::vector<ShaderProgramData::DescriptorSet>& descriptors,
    const std::vector<ShaderProgramData::PushConstantRange>& pushConstants,
    const std::unordered_map<
        vk::ShaderStageFlagBits,
        std::vector<ShaderResourceInterface::ShaderInputInfo>
    >& stageInputs)
    -> std::expected<
        std::unordered_map<vk::ShaderStageFlagBits, std::string>,
        std::string
    >
{
    std::unordered_map<vk::ShaderStageFlagBits, std::string> result;
    for (const auto& [stage, mod] : stages)
    {
        Timer timer;
        shader_edit::ShaderDocument doc = mod.getShaderCode();

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
            if (!(stage & pc.shaderStages)) continue;

            if (auto varName = mod.getPushConstantOffsetPlaceholder(pc.name)) {
                doc.set(*varName, pc.offset);
            }
        }

        // Set input locations in the shader code
        // Currently just uses the default location specified by the generating
        // capability config.
        for (const auto& input : stageInputs.at(stage)) {
            doc.set(input.locationPlaceholder, input.location);
        }

        // Try to finalize GLSL
        try {
            result.emplace(stage, doc.compile());
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

auto collectPushConstants(const ShaderStageMap& stages)
    -> std::vector<ShaderProgramData::PushConstantRange>
{
    using PcRange = ShaderProgramData::PushConstantRange;

    /**
     * Individual semantically atomic push constant values (e.g., the model
     * matrix) can be accessed from multiple shader stages. Here, we find all
     * unique push constants that are accessed across the program and put them
     * at the same offset in each shader stage, so that each value can be
     * uploaded once and then accessed by every stage that requires it.
     */
    std::unordered_map<std::string, PcRange> uniquePushConstants;

    ui32 totalOffset{ 0 };
    for (const auto& [stage, shader] : stages)
    {
        for (const auto& pc : shader.getPushConstants())
        {
            auto [it, isNewUniquePc] = uniquePushConstants.try_emplace(pc.name, PcRange{
                .offset=totalOffset,
                /**
                 * The 16-byte padding is, strictly speaking, slightly over-secure.
                 * Theoretically, each member must be offset by a multiple of its own
                 * alignment. However, I don't want to figure out the the first member
                 * in the next push constant range and deduce its alignment from its
                 * type right now. 16 bytes are the largest possible alignment (e.g. of
                 * 4x4 matrices) and it always works.
                 */
                .size=util::pad_16(pc.size),
                .shaderStages=stage,
                .name=pc.name,
            });

            auto& uniquePc = it->second;
            if (isNewUniquePc) {
                totalOffset += uniquePc.size;
            }
            else {
                if (uniquePc.size != pc.size)
                {
                    throw std::runtime_error(std::format(
                        "Multiple conflicting definitions of push constant \"{}\""
                        " detected: One with size {} bytes, another with size {} bytes.",
                        pc.name, uniquePc.size, pc.size
                    ));
                }

                uniquePc.shaderStages |= stage;
            }
        }
    }

    return uniquePushConstants
        | std::views::values
        | std::ranges::to<std::vector>();
}

/**
 * Combine all push constant ranges across shader stages into one single push
 * constant range.
 */
auto combinePushConstants(const std::vector<ShaderProgramData::PushConstantRange>& pushConstants)
    -> vk::PushConstantRange
{
    vk::PushConstantRange totalRange;
    for (const auto& uniquePc : pushConstants)
    {
        totalRange.offset = glm::min(totalRange.offset, uniquePc.offset);
        totalRange.size += uniquePc.size;
        totalRange.stageFlags |= uniquePc.shaderStages;
    }

    return totalRange;
}

auto applyInputLocationCorrections(
    const ShaderStageMap& stages,
    const ShaderProgramLinkSettings& config)
    -> std::unordered_map<vk::ShaderStageFlagBits, std::vector<ShaderResourceInterface::ShaderInputInfo>>
{
    std::unordered_map<
        vk::ShaderStageFlagBits,
        std::vector<ShaderResourceInterface::ShaderInputInfo>
    > result;

    for (const auto& [stage, mod] : stages)
    {
        auto [it, _] = result.try_emplace(stage, mod.getRequiredShaderInputs());
        if (config.inputLocationMapping.contains(stage))
        {
            auto newLocs = std::ranges::to<std::unordered_map>(config.inputLocationMapping.at(stage));
            auto hasNewLoc = [&](auto&& in){ return newLocs.contains(in.location); };
            for (auto& input : it->second | std::views::filter(hasNewLoc)) {
                input.location = newLocs.at(input.location);
            }
        }
    }

    return result;
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
    data.physicalPushConstantRange = combinePushConstants(data.pushConstants);
    data.descriptorSets = collectDescriptorSets(stages, config);
    auto stageInputs = applyInputLocationCorrections(stages, config);

    // Compile each shader module to SPIR-V
    auto prog = compileProgramCode(stages, data.descriptorSets, data.pushConstants, stageInputs);
    if (prog) {
        data.glslCode = *prog;
    }
    else {
        return std::unexpected(ShaderProgramLinkError{ prog.error() });
    }

    return data;
}



auto ShaderProgramData::serialize() const -> serial::ShaderProgram
{
    serial::ShaderProgram prog;
    for (const auto& [stage, moduleCode] : glslCode)
    {
        auto newModule = prog.add_shader_modules();
        newModule->set_code(moduleCode.data(), moduleCode.size());
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
        pc->set_shader_stage_flags(static_cast<ui32>(range.shaderStages));
        pc->set_name(range.name);
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
        code.resize(mod.code().size());
        memcpy(code.data(), mod.code().data(), mod.code().size());

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
            .shaderStages=vk::ShaderStageFlagBits(range.shader_stage_flags()),
            .name=range.name(),
        });
    }
    physicalPushConstantRange = combinePushConstants(pushConstants);

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
