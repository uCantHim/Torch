#include "trc/material/ShaderResourceInterface.h"

#include <sstream>



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



struct TranslateResource
{
    auto operator()(const ShaderCapabilityConfig::DescriptorBinding& binding)
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
        ss << ";\n";

        return { ss.str(), binding.descriptorName };
    }

    auto operator()(const ShaderCapabilityConfig::VertexInput& in)
        -> std::pair<std::string, std::string>
    {
        auto code = "layout (location = 0) in VertexData\n{\n" + in.contents + "\n} vert;\n";
        return { code, "vert" };
    }

    auto operator()(const ShaderCapabilityConfig::PushConstant& pc)
        -> std::pair<std::string, std::string>
    {
        auto code = "layout (push_constant) uniform PushConstants\n{\n"
                    + pc.contents
                    + "\n} pushConstants;\n";
        return { code, "pushConstants" };
    }

private:
    auto getSetIndex(const std::string& set) -> ui32
    {
        auto [it, success] = setIndices.try_emplace(set, nextSetIndex);
        if (success) {
            ++nextSetIndex;
        }
        return it->second;
    }

    auto makeBindingIndex(const std::string& set) -> ui32
    {
        auto [it, _] = bindingIndices.try_emplace(set, 0);
        return it->second++;
    }

    ui32 nextSetIndex{ 0 };
    std::unordered_map<std::string, ui32> setIndices;
    std::unordered_map<std::string, ui32> bindingIndices;
};



ShaderResourceInterface::ShaderResourceInterface(const ShaderCapabilityConfig& config)
    :
    config(config)
{
}

auto ShaderResourceInterface::compile() const -> ShaderResources
{
    TranslateResource translator;

    std::stringstream ss;

    // Write specialization constants
    for (const auto& [index, name] : specializationConstants) {
        ss << "layout (constant_id = " << index << ") const uint " << name << " = 0;\n";
    }
    ss << "\n";

    // Write capability resources (descriptors, push constants, ...)
    for (auto [capability, code] : capabilityCode) {
        ss << code << "\n";
    }

    // Write constants
    for (const auto& [name, value] : constants) {
        ss << "#define " << name << " (" << value << ")\n";
    }

    // Create result value
    ShaderResources result;
    result.code = ss.str();
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
    TranslateResource translator;

    if (auto res = config.getResource(capability))
    {
        assert(*res != nullptr);
        auto [code, accessor] = std::visit(translator, **res);

        capabilityCode[capability] = std::move(code);
        auto [it, success] = accessors.try_emplace(capability, std::move(accessor));

        return it->second;
    }
    else {
        throw std::runtime_error("Required shader capability " + to_string(capability)
                                 + " is not implemented!");
    }
}

} // namespace trc
