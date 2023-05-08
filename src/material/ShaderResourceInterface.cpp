#include "trc/material/ShaderResourceInterface.h"

#include <sstream>

#include <trc_util/Util.h>

#include "trc/material/FragmentShader.h"



namespace trc
{

auto ShaderResources::getGlslCode() const -> const std::string&
{
    return code;
}

auto ShaderResources::getRequiredShaderInputs() const
    -> const std::vector<ShaderInputInfo>&
{
    return requiredShaderInputs;
}

auto ShaderResources::getSpecializationConstants() const
    -> const std::vector<SpecializationConstantInfo>&
{
    return specConstants;
}

auto ShaderResources::getRequiredDescriptorSets() const -> std::vector<std::string>
{
    std::vector<std::string> result;
    result.reserve(descriptorSetIndexPlaceholders.size());
    for (const auto& [name, _] : descriptorSetIndexPlaceholders) {
        result.emplace_back(name);
    }

    return result;
}

auto ShaderResources::getDescriptorIndexPlaceholder(const std::string& setName) const
    -> std::optional<std::string>
{
    auto it = descriptorSetIndexPlaceholders.find(setName);
    if (it != descriptorSetIndexPlaceholders.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto ShaderResources::getPushConstantSize() const -> ui32
{
    return pushConstantSize;
}

auto ShaderResources::getPushConstants() const -> std::vector<PushConstantInfo>
{
    std::vector<PushConstantInfo> result;
    result.reserve(pushConstantInfos.size());
    for (const auto& [_, pc] : pushConstantInfos) {
        result.emplace_back(pc);
    }

    return result;
}

auto ShaderResources::getPushConstantInfo(ResourceID resource) const
    -> std::optional<PushConstantInfo>
{
    if (pushConstantInfos.contains(resource)) {
        return pushConstantInfos.at(resource);
    }
    return std::nullopt;
}



auto ShaderResourceInterface::DescriptorBindingFactory::make(
    const ShaderCapabilityConfig::DescriptorBinding& binding) -> std::string
{
    const auto placeholder = getDescriptorSetPlaceholder(binding.setName);
    auto [_, success] = descriptorSetPlaceholders.try_emplace(binding.setName, placeholder);
    if (success) {
        generatedCode << "#define _" << placeholder << " $" << placeholder << "\n";
    }

    auto& ss = generatedCode;
    ss << "layout (set = _" << placeholder << " "
       << ", binding = " << binding.bindingIndex;
    if (binding.layoutQualifier) {
        ss << ", " << *binding.layoutQualifier;
    }
    ss << ") " << binding.descriptorType << " "
       << (binding.descriptorName.empty() ? "_" + std::to_string(nextNameIndex++) : binding.descriptorName);
    if (binding.descriptorContent)
    {
        ss << "_Name\n"
           << "{\n" << *binding.descriptorContent << "\n} "
           << binding.descriptorName;
    }
    if (binding.isArray)
    {
        ss << "[";
        if (binding.arrayCount > 0) ss << binding.arrayCount;
        ss << "]";
    }
    ss << ";\n";

    return binding.descriptorName;
}

auto ShaderResourceInterface::DescriptorBindingFactory::getCode() const -> std::string
{
    return generatedCode.str();
}

auto ShaderResourceInterface::DescriptorBindingFactory::getDescriptorSets() const
    -> std::unordered_map<std::string, std::string>
{
    return descriptorSetPlaceholders;
}

auto ShaderResourceInterface::DescriptorBindingFactory::getDescriptorSetPlaceholder(
    const std::string& set)
    -> std::string
{
    return set + "_DESCRIPTOR_SET_INDEX";
}



auto ShaderResourceInterface::PushConstantFactory::make(
    ResourceID resource,
    const ShaderCapabilityConfig::PushConstant& pc) -> std::string
{
    assert(pc.byteSize > 0);
    assert(!pc.typeName.empty());

    infos.try_emplace(resource, PushConstantInfo{ totalSize, pc.byteSize, pc.userId });

    const std::string name = "_push_constant_" + std::to_string(totalSize);
    totalSize += pc.byteSize;
    code += pc.typeName + " " + name + ";\n";

    return "pushConstants." + name;
}

auto ShaderResourceInterface::PushConstantFactory::getCode() const -> std::string
{
    if (!code.empty()) {
        return "layout (push_constant) uniform PushConstants\n{\n" + code + "} pushConstants;\n";
    }
    return "";
}

auto ShaderResourceInterface::PushConstantFactory::getTotalSize() const -> ui32
{
    return totalSize;
}

auto ShaderResourceInterface::PushConstantFactory::getInfos() const
    -> const std::unordered_map<ResourceID, PushConstantInfo>&
{
    return infos;
}



auto ShaderResourceInterface::ShaderInputFactory::make(
    Capability capability,
    const ShaderCapabilityConfig::ShaderInput& in) -> std::string
{
    // const ui32 shaderInputLocation = nextShaderInputLocation;
    // nextShaderInputLocation += in.type.locations();
    const ui32 shaderInputLocation = in.location;
    const std::string name = "shaderStageInput_" + std::to_string(shaderInputLocation);

    // Make member code
    std::stringstream ss;
    ss << (in.flat ? "flat " : "") << in.type.to_string() << " " << name;
    shaderInputs.push_back({ shaderInputLocation, in.type, name, ss.str(), capability });

    return name;
}



ShaderResourceInterface::ShaderResourceInterface(
    const ShaderCapabilityConfig& config,
    ShaderCodeBuilder& codeBuilder)
    :
    config(config),
    codeBuilder(&codeBuilder),
    requiredExtensions(config.getGlobalShaderExtensions()),
    requiredIncludePaths(config.getGlobalShaderIncludes())
{
}

auto ShaderResourceInterface::compile() const -> ShaderResources
{
    std::stringstream ss;

    for (const auto& [_, macro] : resourceMacros) {
        ss << "#define " << macro.first << " " << macro.second << "\n";
    }
    for (const auto& [name, val] : requiredMacros) {
        ss << "#define " << name << " (" << val.value_or("") << ")\n";
    }
    for (const auto& ext : requiredExtensions) {
        ss << "#extension " << ext << " : require\n";
    }
    for (const auto& path : requiredIncludePaths) {
        ss << "#include \"" << path.string() << "\"\n";
    }
    ss << "\n";

    // Write specialization constants
    for (const auto& [index, name] : specializationConstants) {
        ss << "layout (constant_id = " << index << ") const uint " << name << " = 0;\n";
    }
    ss << "\n";

    // Write descriptor sets
    ss << descriptorFactory.getCode() << "\n";

    // Write push constants
    ss << pushConstantFactory.getCode() << "\n";

    // Write shader inputs
    for (const auto& out : shaderInput.shaderInputs) {
        ss << "layout (location = " << out.location << ") in " << out.declCode << ";\n";
    }
    ss << "\n";

    // Create result value
    ShaderResources result;
    result.code = ss.str();
    result.requiredShaderInputs = shaderInput.shaderInputs;
    for (const auto& [specIdx, value] : specializationConstantValues)
    {
        result.specConstants.push_back(ShaderResources::SpecializationConstantInfo{
            .value=value,
            .specializationConstantIndex=specIdx
        });
    }
    result.descriptorSetIndexPlaceholders = descriptorFactory.getDescriptorSets();
    result.pushConstantSize = pushConstantFactory.getTotalSize();
    result.pushConstantInfos = pushConstantFactory.getInfos();

    return result;
}

auto ShaderResourceInterface::queryCapability(Capability capability) -> code::Value
{
    for (auto id : config.getCapabilityResources(capability)) {
        requireResource(capability, id);
    }

    return config.accessCapability(capability);
}

auto ShaderResourceInterface::makeSpecConstant(s_ptr<ShaderRuntimeConstant> value) -> code::Value
{
    auto [it, success] = specializationConstantValues.try_emplace(nextSpecConstantIndex++, value);
    assert(success);

    const auto& [idx, _] = *it;
    const std::string specConstName = "kSpecConstant" + std::to_string(idx) + "_RuntimeValue";
    specializationConstants.emplace_back(idx, specConstName);

    // Create the shader code value
    auto id = codeBuilder->makeExternalIdentifier(specConstName);
    codeBuilder->annotateType(id, value->getType());

    return id;
}

void ShaderResourceInterface::requireResource(
    Capability capability,
    ShaderCapabilityConfig::ResourceID resourceId)
{
    const auto& res = config.getResource(resourceId);

    // Only add a resource once
    if (resourceMacros.contains(&res)) {
        return;
    }

    // First resource access; create it.
    requiredExtensions.insert(res.extensions.begin(), res.extensions.end());
    requiredIncludePaths.insert(res.includeFiles.begin(), res.includeFiles.end());
    for (const auto& [name, val] : res.macroDefinitions) {
        requiredMacros.emplace_back(name, val);
    }

    auto accessorStr = std::visit(util::VariantVisitor{
        [this](const ShaderCapabilityConfig::DescriptorBinding& binding) {
            return descriptorFactory.make(binding);
        },
        [this, capability](const ShaderCapabilityConfig::ShaderInput& v) {
            return shaderInput.make(capability, v);
        },
        [this, resourceId](const ShaderCapabilityConfig::PushConstant& pc) {
            return pushConstantFactory.make(resourceId, pc);
        }
    }, res.resourceType);

    resourceMacros.try_emplace(&res, res.resourceMacroName, accessorStr);
}

} // namespace trc
