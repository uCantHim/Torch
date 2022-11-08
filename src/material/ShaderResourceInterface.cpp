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



auto ShaderResourceInterface::DescriptorBindingFactory::make(
    const ShaderCapabilityConfig::DescriptorBinding& binding) -> std::string
{
    auto& ss = generatedCode;
    ss << "layout (set = " << getSetIndex(binding.setName)
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

auto ShaderResourceInterface::DescriptorBindingFactory::getOrderedDescriptorSets() const
    -> std::vector<std::string>
{
    std::vector<std::string> vec(setIndices.size());
    for (const auto& [set, idx] : setIndices) {
        vec.at(idx) = set;
    }
    return vec;
}

auto ShaderResourceInterface::DescriptorBindingFactory::getSetIndex(const std::string& set) -> ui32
{
    auto [it, success] = setIndices.try_emplace(set, nextSetIndex);
    if (success) {
        ++nextSetIndex;
    }
    return it->second;
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



ShaderResourceInterface::ShaderResourceInterface(const ShaderCapabilityConfig& config)
    :
    config(config)
{
}

auto ShaderResourceInterface::compile() const -> ShaderResources
{
    std::stringstream ss;

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

    // Write constants
    for (const auto& [name, value] : constants) {
        ss << "#define " << name << " (" << value << ")\n";
    }

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

    return result;
}

auto ShaderResourceInterface::makeScalarConstant(Constant constantValue) -> std::string
{
    const auto name = "kConstant" + std::to_string(nextConstantId++) + "_" + constantValue.datatype();
    constants.try_emplace(name, constantValue);

    return name;
}

auto ShaderResourceInterface::queryTexture(TextureReference tex) -> std::string
{
    auto [it, success] = specializationConstantTextures.try_emplace(nextSpecConstantIndex++, tex);
    assert(success);

    auto [idx, _] = *it;
    std::string specConstName = "kSpecConstant" + std::to_string(idx) + "_TextureIndex";
    specializationConstants.emplace_back(idx, specConstName);

    return hardcoded_makeTextureAccessor(specConstName);
}

auto ShaderResourceInterface::queryCapability(Capability capability) -> std::string
{
    return accessCapability(capability);
}

auto ShaderResourceInterface::hardcoded_makeTextureAccessor(const std::string& textureIndexName)
    -> std::string
{
    return accessCapability(FragmentCapability::kTextureSample) + "[" + textureIndexName + "]";
}

auto ShaderResourceInterface::accessCapability(Capability capability) -> std::string
{
    if (auto resource = config.getResource(capability))
    {
        assert(*resource != nullptr);
        auto& res = **resource;

        requiredExtensions.insert(res.extensions.begin(), res.extensions.end());
        requiredIncludePaths.insert(res.includeFiles.begin(), res.includeFiles.end());

        auto accessor = std::visit(util::VariantVisitor{
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

        if (auto appendage = config.getCapabilityAccessor(capability)) {
            accessor += *appendage;
        }

        auto [it, success] = capabilityAccessors.try_emplace(capability, std::move(accessor));
        return it->second;
    }
    else {
        throw std::runtime_error("Required shader capability " + capability.getString()
                                 + " is not implemented!");
    }
}

} // namespace trc
