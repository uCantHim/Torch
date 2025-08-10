#include "trc/material/ShaderStageInputLinker.h"

#include <cassert>

#include <algorithm>
#include <ranges>

#include "trc/material/shader/ShaderModuleBuilder.h"
#include "trc/material/shader/ShaderOutputInterface.h"



namespace trc
{

constexpr bool orderStages(vk::ShaderStageFlagBits a, vk::ShaderStageFlagBits b)
{
    // Improvement: Order only the valid rasterization stages
    return static_cast<int>(a) < static_cast<int>(b);
}

auto linkShaderStageInputs(
    const std::unordered_map<vk::ShaderStageFlagBits, ModuleLinkInfo>& modules)
    -> std::expected<ShaderStageInputLinkResult, ShaderStageInputLinkError>
{
    using Pair = std::pair<vk::ShaderStageFlagBits, ModuleLinkInfo>;

    auto stages = std::ranges::to<std::vector<Pair>>(modules);
    std::ranges::sort(stages, [](Pair& a, Pair& b) { return orderStages(a.first, b.first); });

    std::unordered_map<vk::ShaderStageFlagBits, std::vector<std::pair<ui32, ui32>>> locationMap;
    std::unordered_map<
        vk::ShaderStageFlagBits,
        std::vector<shader::ShaderResourceInterface::ShaderInputInfo>
    > remainingInputs;

    for (int i = stages.size() - 1; i >= 0; --i)
    {
        auto [curStage, curModule] = stages[i];
        assert(curModule.builder != nullptr);

        if (i > 0)
        {
            // Add the current stage's inputs to the list of required inputs
            remainingInputs.try_emplace(
                curStage,
                curModule.builder->getResourceInterface().getRequiredShaderInputs());

            // Try to get the required shader inputs from the preceding stage
            auto [prevStage, prevModule] = stages[i - 1];
            assert(prevModule.builder != nullptr);
            assert(prevModule.outputs != nullptr);
            for (auto& [inputStage, requiredInputs] : remainingInputs)
            {
                for (auto it = requiredInputs.begin(); it != requiredInputs.end(); /*nothing*/)
                {
                    const auto& reqInput = *it;
                    const ui32 locIdx = prevModule.nextOutputLocation++;
                    try {
                        auto capValue = prevModule.builder->makeCapabilityAccess(reqInput.capability);
                        auto location = prevModule.builder->makeOutputLocation(locIdx, reqInput.type);
                        prevModule.outputs->makeStore(location, capValue);
                    }
                    catch (const std::out_of_range&) {
                        ++it;
                        continue;
                    }

                    // Remap current module's input location if necessary
                    if (locIdx != reqInput.location)
                    {
                        auto [locMapIt, _] = locationMap.try_emplace(inputStage);
                        locMapIt->second.emplace_back(reqInput.location, locIdx);
                    }

                    it = requiredInputs.erase(it);
                }
            }
        }
    }

    // Validate
    const auto numUnresolved = std::ranges::fold_left(
        remainingInputs | std::views::transform([](auto& pair){ return pair.second.size(); }),
        0, std::plus{}
    );
    if (numUnresolved > 0) {
        return std::unexpected(ShaderStageInputLinkError{ std::move(remainingInputs) });
    }

    // Create result
    return ShaderStageInputLinkResult{
        .locationMap = std::move(locationMap),
    };
}

} // namespace trc
