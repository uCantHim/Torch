#include "trc/material/ShaderResourceInterface.h"

#include <sstream>

#include <trc_util/Util.h>



namespace trc
{

auto ShaderResources::getGlslCode() const -> const std::string&
{
    return code;
}

auto ShaderResources::getReferencedTextures() const -> const std::vector<TextureResource>&
{
    return textures;
}

auto ShaderResources::getShaderInputs() const
    -> const std::vector<ShaderInputInfo>&
{
    return requiredShaderInputs;
}

auto ShaderResources::getDescriptorSets() const -> std::vector<std::string>
{
    std::vector<std::string> result;
    result.reserve(descriptorSetIndexPlaceholders.size());
    for (const auto& [name, _] : descriptorSetIndexPlaceholders) {
        result.emplace_back(name);
    }

    return result;
}

auto ShaderResources::getDescriptorSetIndexPlaceholder(const std::string& setName) const
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



auto ShaderResourceInterface::DescriptorBindingFactory::make(
    const ShaderCapabilityConfig::DescriptorBinding& binding) -> std::string
{
    const auto placeholder = getDescriptorSetPlaceholder(binding.setName);
    descriptorSetPlaceholders.try_emplace(binding.setName, placeholder);

    auto& ss = generatedCode;
    ss << "layout (set = $" << placeholder << " "
       << ", binding = " << binding.bindingIndex;
    if (binding.layoutQualifier) {
        ss << ", " << *binding.layoutQualifier;
    }
    ss << ") " << binding.descriptorType << " " << binding.descriptorName;
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
    const ShaderCapabilityConfig::PushConstant& pc) -> std::string
{
    if (!code.empty()) {
        throw std::runtime_error("[In PushConstantFactory::make]: At most one push constant"
                                 " resource may be specified at the shader capability config!");
    }

    code = "layout (push_constant) uniform PushConstants\n{\n"
                + pc.contents
                + "\n} pushConstants;\n";
    return "pushConstants";
}

auto ShaderResourceInterface::PushConstantFactory::getCode() const -> const std::string&
{
    return code;
}



auto ShaderResourceInterface::ShaderInputFactory::make(
    Capability capability,
    const ShaderCapabilityConfig::ShaderInput& in) -> std::string
{
    const ui32 shaderInputLocation = nextShaderInputLocation;
    nextShaderInputLocation += in.type.locations();
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
    for (const auto& [specIdx, texRef] : specializationConstantTextures)
    {
        result.textures.push_back(ShaderResources::TextureResource{
            .ref=texRef,
            .specializationConstantIndex=specIdx
        });
    }
    result.descriptorSetIndexPlaceholders = descriptorFactory.getDescriptorSets();

    return result;
}

auto ShaderResourceInterface::queryTexture(TextureReference tex) -> code::Value
{
    auto [it, success] = specializationConstantTextures.try_emplace(nextSpecConstantIndex++, tex);
    assert(success);

    auto [idx, _] = *it;
    const std::string specConstName = "kSpecConstant" + std::to_string(idx) + "_TextureIndex";
    specializationConstants.emplace_back(idx, specConstName);

    return codeBuilder->makeArrayAccess(
        queryCapability(FragmentCapability::kTextureSample),
        codeBuilder->makeExternalIdentifier(specConstName)
    );
}

auto ShaderResourceInterface::queryCapability(Capability capability) -> code::Value
{
    for (auto id : config.getCapabilityResources(capability)) {
        requireResource(capability, &config.getResource(id));
    }

    return config.accessCapability(capability);
}

void ShaderResourceInterface::requireResource(Capability capability, Resource resource)
{
    assert(resource != nullptr);

    // Only add a resource once
    if (resourceMacros.contains(resource)) {
        return;
    }

    // First resource access; create it.
    auto& res = *resource;
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
        [this](const ShaderCapabilityConfig::PushConstant& pc) {
            return pushConstantFactory.make(pc);
        }
    }, res.resourceType);

    resourceMacros.try_emplace(resource, res.resourceMacroName, accessorStr);
}

} // namespace trc
