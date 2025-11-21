#include "trc/material/shader/ShaderRuntime.h"

#include <algorithm>

#include "trc/material/shader/ShaderProgram.h"



namespace trc::shader
{

ShaderProgramRuntime::ShaderProgramRuntime(const ShaderProgramData& program)
    :
    allStages(program.physicalPushConstantRange.stageFlags)
{
    std::vector<PushConstantRange> _pc;
    std::unordered_map<std::string, ui32> _descriptorSetIndices;
    std::unordered_map<std::string, PushConstant> _pcHandles;

    for (auto [offset, size, stages, name] : program.pushConstants)
    {
        const ui32 nextPcIdx = _pc.size();
        _pc.emplace_back( PushConstantRange{
            .offset=offset,
            .size=size,
            .stages=stages,
        });
        _pcHandles.try_emplace(name, PushConstant{ name, nextPcIdx });
    }

    for (const auto& [desc, index] : program.descriptorSets) {
        _descriptorSetIndices.try_emplace(desc, index);
    }

    this->pc = std::make_shared<std::vector<PushConstantRange>>(std::move(_pc));
    this->descriptorSetIndices = std::make_shared<std::unordered_map<std::string, ui32>>(
        std::move(_descriptorSetIndices));
    this->pcHandlesByName = std::make_shared<std::unordered_map<std::string, PushConstant>>(
        std::move(_pcHandles));
}

auto ShaderProgramRuntime::clone() const -> u_ptr<ShaderProgramRuntime>
{
    return std::make_unique<ShaderProgramRuntime>(*this);
}

auto ShaderProgramRuntime::getPushConstantHandle(const std::string& name) -> PushConstant
{
    try {
        return pcHandlesByName->at(name);
    }
    catch (const std::out_of_range&) {
        throw std::out_of_range("[In ShaderProgramRuntime::getPushConstantHandle]:"
                                " Push constant \"" + name + "\" is not present in the program.");
    }
}

auto ShaderProgramRuntime::getPushConstantHandle(std::string_view name) -> PushConstant
{
    return getPushConstantHandle(std::string{ name });
}

auto ShaderProgramRuntime::tryGetPushConstantHandle(const std::string& name) -> std::optional<PushConstant>
{
    auto it = pcHandlesByName->find(name);
    if (it != pcHandlesByName->end()) {
        return it->second;
    }
    return std::nullopt;
}

auto ShaderProgramRuntime::tryGetPushConstantHandle(std::string_view name) -> std::optional<PushConstant>
{
    return tryGetPushConstantHandle(std::string{ name });
}

auto ShaderProgramRuntime::getDescriptorSetIndex(const std::string& name) -> ui32
{
    try {
        return descriptorSetIndices->at(name);
    }
    catch (const std::out_of_range&) {
        throw std::out_of_range("[In ShaderProgramRuntime::getDescriptorHandle]:"
                                " Descriptor \"" + name + "\" is not present in the program.");
    }
}

auto ShaderProgramRuntime::getDescriptorSetIndex(std::string_view name) -> ui32
{
    return getDescriptorSetIndex(std::string{ name });
}

auto ShaderProgramRuntime::tryGetDescriptorSetIndex(const std::string& name) -> std::optional<ui32>
{
    auto it = descriptorSetIndices->find(name);
    if (it != descriptorSetIndices->end()) {
        return it->second;
    }
    return std::nullopt;
}

auto ShaderProgramRuntime::tryGetDescriptorSetIndex(std::string_view name) -> std::optional<ui32>
{
    return tryGetDescriptorSetIndex(std::string{ name });
}

void ShaderProgramRuntime::pushConstants(
    vk::CommandBuffer cmdBuf,
    vk::PipelineLayout layout,
    PushConstant pcHandle,
    const void* data, size_t size) const
{
    assert_arg(exists(pcHandle));

    const ui32 id = pcHandle.getInternalId();
    doPushConstants(cmdBuf, layout, id, data, size);
}

void ShaderProgramRuntime::setPushConstantDefaultValue(
    PushConstant pcHandle,
    std::span<const std::byte> data)
{
    assert_arg(exists(pcHandle));

    const ui32 id = pcHandle.getInternalId();
    auto it = std::ranges::find_if(pushConstantData, [&](auto& p){ return p.first == id; });
    if (it != pushConstantData.end()) {
        it->second = std::vector<std::byte>{ data.begin(), data.end() };
    }
    else {
        pushConstantData.emplace_back(
            pcHandle.getInternalId(),
            std::vector<std::byte>{ data.begin(), data.end() }
        );
    }
}

void ShaderProgramRuntime::uploadPushConstantDefaultValues(
    vk::CommandBuffer cmdBuf,
    vk::PipelineLayout layout)
{
    for (const auto& [id, data] : pushConstantData) {
        doPushConstants(cmdBuf, layout, id, data.data(), data.size());
    }
}

bool ShaderProgramRuntime::exists(const PushConstant& hnd) const
{
    return pc->size() > hnd.getInternalId()
        && pcHandlesByName->contains(hnd.getName());
}

void ShaderProgramRuntime::doPushConstants(
    vk::CommandBuffer cmdBuf,
    vk::PipelineLayout layout,
    ui32 internalId,
    const void* data, size_t size) const
{
    cmdBuf.pushConstants(layout,
                         allStages,
                         pc->at(internalId).offset,
                         size,
                         data);
}

} // namespace trc::shader
