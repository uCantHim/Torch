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



auto ShaderResourceInterface::TranslateResource::operator()(const ShaderCapabilityConfig::DescriptorBinding& binding)
    -> std::pair<std::string, std::string>
{
    std::stringstream ss;
    ss << "layout (set = " << getSetIndex(binding.setName)
       << ", binding = " << makeBindingIndex(binding.setName);
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
    ss << ";";

    return { ss.str(), binding.descriptorName };
}

auto ShaderResourceInterface::TranslateResource::operator()(
    const ShaderCapabilityConfig::PushConstant& pc)
    -> std::pair<std::string, std::string>
{
    auto code = "layout (push_constant) uniform PushConstants\n{\n"
                + pc.contents
                + "\n} pushConstants;\n";
    return { code, "pushConstants" };
}

auto ShaderResourceInterface::TranslateResource::getSetIndex(const std::string& set) -> ui32
{
    auto [it, success] = setIndices.try_emplace(set, nextSetIndex);
    if (success) {
        ++nextSetIndex;
    }
    return it->second;
}

auto ShaderResourceInterface::TranslateResource::makeBindingIndex(const std::string& set) -> ui32
{
    auto [it, _] = bindingIndices.try_emplace(set, 0);
    return it->second++;
}



auto ShaderResourceInterface::ShaderInputFactory::make(
    Capability capability,
    const ShaderCapabilityConfig::ShaderInput& in) -> std::string
{
    const ui32 shaderInputLocation = nextShaderInputLocation++;
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

    // Write specialization constants
    for (const auto& [index, name] : specializationConstants) {
        ss << "layout (constant_id = " << index << ") const uint " << name << " = 0;\n";
    }
    ss << "\n";

    // Write capability resources (descriptors, push constants, ...)
    for (auto [resource, code] : resourceCode) {
        ss << code << "\n";
    }
    ss << "\n";

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

auto ShaderResourceInterface::queryConstant(Builtin constantType) -> std::string
{
    return accessCapability(config.getConstantCapability(constantType))
         + config.getConstantAccessor(constantType).value_or("");
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

auto ShaderResourceInterface::getConstantType(Builtin constant) -> BasicType
{
    return config.getConstantType(constant);
}

auto ShaderResourceInterface::hardcoded_makeTextureAccessor(const std::string& textureIndexName)
    -> std::string
{
    return accessCapability(Capability::eTextureSample) + "[" + textureIndexName + "]";
}

auto ShaderResourceInterface::accessCapability(Capability capability) -> std::string
{
    if (auto resource = config.getResource(capability))
    {
        assert(*resource != nullptr);

        auto accessor = std::visit(util::VariantVisitor{
            [this, capability](const ShaderCapabilityConfig::ShaderInput& v) {
                auto accessor = shaderInput.make(capability, v);
                return accessor;
            },
            [this, &resource](const auto& t){
                auto [code, accessor] = resourceTranslator(t);
                resourceCode[*resource] = std::move(code);
                return accessor;
            }
        }, **resource);

        auto [it, success] = capabilityAccessors.try_emplace(capability, std::move(accessor));
        return it->second;
    }
    else {
        throw std::runtime_error("Required shader capability " + to_string(capability)
                                 + " is not implemented!");
    }
}

} // namespace trc
