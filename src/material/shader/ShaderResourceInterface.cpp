#include "trc/material/shader/ShaderResourceInterface.h"

#include <algorithm>
#include <format>
#include <ranges>
#include <sstream>

#include <trc_util/Util.h>

#include "trc/base/Logging.h"
#include "trc/material/shader/ShaderModuleBuilder.h"



namespace trc::shader
{

auto ShaderResourceInterface::getRequiredShaderInputs() const
    -> const std::vector<ShaderInputInfo>&
{
    return requiredShaderInputs;
}

auto ShaderResourceInterface::getSpecializationConstants() const
    -> const std::vector<SpecializationConstantInfo>&
{
    return specConstants;
}

auto ShaderResourceInterface::getRequiredDescriptorSets() const -> std::vector<std::string>
{
    std::vector<std::string> result;
    result.reserve(descriptorSetIndexPlaceholders.size());
    for (const auto& [name, _] : descriptorSetIndexPlaceholders) {
        result.emplace_back(name);
    }

    return result;
}

auto ShaderResourceInterface::getDescriptorIndexPlaceholder(const std::string& setName) const
    -> std::optional<std::string>
{
    auto it = descriptorSetIndexPlaceholders.find(setName);
    if (it != descriptorSetIndexPlaceholders.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto ShaderResourceInterface::getPushConstantSize() const -> ui32
{
    return pushConstantSize;
}

auto ShaderResourceInterface::getPushConstants() const -> std::vector<PushConstantInfo>
{
    std::vector<PushConstantInfo> result;
    result.reserve(pushConstantInfos.size());
    for (const auto& [_, pc] : pushConstantInfos) {
        result.emplace_back(pc);
    }

    return result;
}

auto ShaderResourceInterface::getPushConstantOffsetPlaceholder(
    const std::string& pushConstantName) const
    -> std::optional<std::string>
{
    if (pushConstantInfos.contains(pushConstantName)) {
        return pushConstantInfos.at(pushConstantName).offsetPlaceholder;
    }
    return std::nullopt;
}

auto ShaderResourceInterface::getRequiredPayloads() const -> const std::vector<PayloadInfo>&
{
    return requiredPayloads;
}

auto ShaderResourceInterface::serialize() const -> serial::ShaderResourceInterface
{
    serial::ShaderResourceInterface res;

    res.set_code(code);
    res.set_total_pc_size(pushConstantSize);
    for (const auto& [key, value] : descriptorSetIndexPlaceholders) {
        res.mutable_desc_index_placholders()->try_emplace(key, value);
    }

    for (const auto& in : requiredShaderInputs)
    {
        auto _in = res.add_shader_inputs();
        _in->set_location(in.location);
        auto type = _in->mutable_type();
        type->set_type(static_cast<ui32>(in.type.type));
        type->set_channels(in.type.channels);
        _in->set_shader_var_id(in.variableName);
        _in->set_capability(in.capability.toString());
        _in->set_location_placeholder(in.locationPlaceholder);
    }
    for (const auto& pl : requiredPayloads)
    {
        auto _pl = res.add_payloads();
        _pl->set_capability(pl.capability.toString());
        _pl->set_location_placeholder(pl.locationPlaceholder);
        if (auto type = std::get_if<BasicType>(&pl.type))
        {
            auto _type = _pl->mutable_basic_type();
            _type->set_type(static_cast<ui32>(type->type));
            _type->set_channels(type->channels);
        }
        else {
            log::error << "[ShaderResourceInterface::serialize]: Unable to serialize struct type"
                          " of shader payload: not implemented.";
        }
    }
    for (const auto& spec : specConstants)
    {
        auto _spec = res.add_spec_constants();
        _spec->set_index(spec.specializationConstantIndex);
        auto val = _spec->mutable_value();
        val->set_data(spec.value->serialize());
    }
    for (const auto& [_, pc] : pushConstantInfos)
    {
        auto _pc = res.add_push_constants();
        _pc->set_offset(pc.offset);
        _pc->set_size(pc.size);
        _pc->set_offset_placeholder(pc.offsetPlaceholder);
        _pc->set_name(pc.name);
    }

    return res;
}

auto ShaderResourceInterface::deserialize(
    const serial::ShaderResourceInterface& data,
    ShaderRuntimeConstantDeserializer* des)
    -> ShaderResourceInterface
{
    ShaderResourceInterface res;

    res.code = data.code();
    res.pushConstantSize = data.total_pc_size();
    const auto& _desc_index_ph = data.desc_index_placholders();
    res.descriptorSetIndexPlaceholders = { _desc_index_ph.begin(), _desc_index_ph.end() };

    for (const auto& in : data.shader_inputs())
    {
        res.requiredShaderInputs.push_back({
            .location = in.location(),
            .type{
                static_cast<BasicType::Type>(in.type().type()),
                static_cast<ui8>(in.type().channels())
            },
            .variableName = in.shader_var_id(),
            .capability = in.capability(),
            .locationPlaceholder = in.location_placeholder(),
        });
    }
    for (const auto& pl : data.payloads())
    {
        assert(pl.has_basic_type() && "alternative is not implemented.");
        res.requiredPayloads.push_back({
            .type = BasicType{
                static_cast<BasicType::Type>(pl.basic_type().type()),
                static_cast<ui8>(pl.basic_type().channels())
            },
            .capability = pl.capability(),
            .locationPlaceholder = pl.location_placeholder(),
        });
    }
    if (des != nullptr)
    {
        for (const auto& spec : data.spec_constants())
        {
            res.specConstants.push_back({
                .value = des->deserialize(spec.value().data()).value(),
                .specializationConstantIndex = spec.index(),
            });
        }
    }
    else if (data.spec_constants_size() > 0) {
        log::warn << "[ShaderResourceInterface::deserialize]: Loaded data has "
                  << data.spec_constants_size() << " runtime constants defined, but no"
                  << " runtime constant deserializer was specified. Runtime constants"
                     " will be ignored.";
    }
    for (const auto& pc : data.push_constants())
    {
        res.pushConstantInfos.try_emplace(pc.name(), PushConstantInfo{
            .offset = pc.offset(),
            .size = pc.size(),
            .offsetPlaceholder = pc.offset_placeholder(),
            .name = pc.name(),
        });
    }

    return res;
}



auto ShaderResourceInterfaceBuilder::DescriptorBindingFactory::make(
    const CapabilityConfig::DescriptorBinding& binding) -> std::string
{
    const auto placeholder = makeDescriptorSetPlaceholder(binding.setName);
    auto [_, success] = resources->descriptorSetIndexPlaceholders.try_emplace(binding.setName,
                                                                              placeholder);
    if (success) {
        generatedCode += "#define _" + placeholder + " $" + placeholder + "\n";
    }

    const auto descriptorName = binding.descriptorName.empty()
            ? "_descriptorName_" + std::to_string(nextNameIndex++)
            : binding.descriptorName;

    auto& ss = generatedCode;
    ss += "layout (set = _" + placeholder + ", binding = " + std::to_string(binding.bindingIndex);
    if (binding.layoutQualifier) {
        ss += ", " + *binding.layoutQualifier;
    }
    ss += ") " + binding.descriptorType + " " + descriptorName;
    if (binding.descriptorContent) {
        ss += "_Name\n{\n" + *binding.descriptorContent + "\n} " + descriptorName;
    }
    if (binding.arrayCount)
    {
        ss += "[";
        if (*binding.arrayCount > 0) ss += std::to_string(*binding.arrayCount);
        ss += "]";
    }
    ss += ";\n";

    return descriptorName;
}

auto ShaderResourceInterfaceBuilder::DescriptorBindingFactory::getCode() const -> const std::string&
{
    return generatedCode;
}

auto ShaderResourceInterfaceBuilder::DescriptorBindingFactory::makeDescriptorSetPlaceholder(
    const std::string& set)
    -> std::string
{
    return set + "_DESCRIPTOR_SET_INDEX";
}



auto ShaderResourceInterfaceBuilder::PushConstantFactory::make(
    ResourceID /*resource*/,
    const CapabilityConfig::PushConstant& pc) -> std::string
{
    const auto byteSize = code::types::getTypeSize(pc.type);
    assert(byteSize > 0);

    const std::string glslName = "_push_constant_" + std::to_string(nextInternalId++);
    const std::string offsetPlaceholder = glslName + "_offset";

    resources->pushConstantInfos.try_emplace(
        pc.name,
        PushConstantInfo{
            .offset=resources->pushConstantSize,
            .size=byteSize,
            .offsetPlaceholder=offsetPlaceholder,
            .name=pc.name,
        }
    );
    resources->pushConstantSize += byteSize;

    code += std::format(
        "layout (offset=${}) {} {};\n",
        offsetPlaceholder,
        code::types::to_string(pc.type),
        glslName
    );

    return std::format("{}.{}", kPcBlockNamespaceName, glslName);
}

auto ShaderResourceInterfaceBuilder::PushConstantFactory::getCode() const -> std::string
{
    if (!code.empty())
    {
        return std::format(
            "layout (push_constant) uniform {} \n{{\n{}\n}} {};",
            kPcBlockName,
            code,
            kPcBlockNamespaceName
        );
    }
    return "";
}



auto ShaderResourceInterfaceBuilder::ShaderInputFactory::make(
    Capability capability,
    const CapabilityConfig::ShaderInput& in) -> std::string
{
    const ui32 shaderInputLocation = in.location;
    const std::string name = "shaderStageInput_" + std::to_string(shaderInputLocation);

    auto& inputDef = resources->requiredShaderInputs.emplace_back(
        ShaderResourceInterface::ShaderInputInfo{
            .location=shaderInputLocation,
            .type=in.type,
            .variableName=name,
            .capability=capability,
            .locationPlaceholder=name + "_LOCATION_PLACEHOLDER"
        }
    );

    // Make member code
    code += std::format(
        "layout (location = {}) in {}{} {};\n",
        "$" + inputDef.locationPlaceholder,
        (in.flat ? "flat " : ""),
        in.type.to_string(),
        name
    );

    return name;
}

auto ShaderResourceInterfaceBuilder::ShaderInputFactory::getCode() const -> std::string
{
    return code;
}



auto ShaderResourceInterfaceBuilder::RayPayloadFactory::make(
    Capability requestingCapability,
    const CapabilityConfig::RayPayload& pl)
    -> std::string
{
    std::string payloadIdentifier = "_ray_payload_" + std::to_string(nextNameIndex++);
    std::string locationPlaceholder = payloadIdentifier + "_LOCATION_PLACEHOLDER";

    resources->requiredPayloads.push_back({
        .type=pl.type,
        .capability=requestingCapability,
        .locationPlaceholder=locationPlaceholder
    });

    code +=
        "layout (location = $" + locationPlaceholder + ") " +
        (pl.callableData ? "callableData" : "rayPayload") + (pl.incoming ? "In" : "") + "EXT"
        + " " + code::types::to_string(pl.type)
        + payloadIdentifier
        + ";\n";

    return payloadIdentifier;
}

auto ShaderResourceInterfaceBuilder::RayPayloadFactory::getCode() const -> const std::string&
{
    return code;
}



auto ShaderResourceInterfaceBuilder::HitAttributeFactory::make(
    const CapabilityConfig::HitAttribute& att)
    -> std::string
{
    const std::string identifier = " _hit_attribute_" + std::to_string(nextNameIndex++);
    code += "hitAttributeEXT " + code::types::to_string(att.type) + identifier;

    return identifier;
}

auto ShaderResourceInterfaceBuilder::HitAttributeFactory::getCode() const -> const std::string&
{
    return code;
}



ShaderResourceInterfaceBuilder::ShaderResourceInterfaceBuilder(
    s_ptr<const CapabilityConfig> config,
    ShaderModuleBuilder& codeBuilder)
    :
    config(config),
    builder(&codeBuilder),
    resources(std::make_shared<ShaderResourceInterface>()),
    requiredExtensions(config->getGlobalShaderExtensions()),
    requiredIncludePaths(config->getGlobalShaderIncludes())
{
}

auto ShaderResourceInterfaceBuilder::getResourceInterface() const
    -> s_ptr<const ShaderResourceInterface>
{
    return resources;
}

auto ShaderResourceInterfaceBuilder::compile() const -> std::string
{
    /**
     * We sort these macro definitions to obtain a deterministic result. This is
     * important for the shader caching system which caches SPIR-V code for the
     * corresponding GLSL code (see ShaderCache).
     */
    auto getOrderedResourceMacros = [this] -> std::vector<std::pair<std::string, std::string>> {
        auto items = resourceMacros | std::views::values | std::ranges::to<std::vector>();
        std::ranges::sort(items);
        return items;
    };

    std::stringstream ss;

    for (const auto& [macroName, resourceAccessor] : getOrderedResourceMacros()) {
        ss << "#define " << macroName << " " << resourceAccessor << "\n";
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
    ss << shaderInputFactory.getCode() << "\n";

    // Write ray payloads
    ss << rayPayloadFactory.getCode() << "\n";

    // Write hit attributes
    ss << hitAttributeFactory.getCode() << "\n";

    return ss.str();
}

auto ShaderResourceInterfaceBuilder::queryCapability(Capability capability) -> code::Value
{
    // TODO: Cache capability values?
    const auto [value, resources] = config->accessCapability(capability, *builder);
    for (auto id : resources) {
        requireResource(capability, id);
    }

    return value;
}

auto ShaderResourceInterfaceBuilder::makeSpecConstant(s_ptr<ShaderRuntimeConstant> value) -> code::Value
{
    // Create resource
    auto spec = resources->specConstants.emplace_back(
        ShaderResourceInterface::SpecializationConstantInfo{
            .value=value,
            .specializationConstantIndex=nextSpecConstantIndex++,
        }
    );

    const ui32 idx = spec.specializationConstantIndex;
    const std::string specConstName = "kSpecConstant" + std::to_string(idx) + "_RuntimeValue";
    specializationConstants.emplace_back(idx, specConstName);

    // Create the shader code value
    auto id = builder->makeExternalIdentifier(specConstName);
    builder->annotateType(id, value->getType());

    return id;
}

void ShaderResourceInterfaceBuilder::requireResource(
    Capability capability,
    CapabilityConfig::ResourceID resourceId)
{
    const auto& res = config->getResourceInfo(resourceId);

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

    auto registerType = [this](code::Type type) {
        // Inform the builder that a struct type exists, because the type
        // may not have been created at a module builder.
        if (std::holds_alternative<code::types::StructType>(type))
        {
            auto structType = std::get<code::types::StructType>(type);
            try {
                builder->makeStructType(structType->name, structType->fields);
            } catch(...){}
        }
    };

    auto accessorStr = std::visit(util::VariantVisitor{
        [this](const CapabilityConfig::DescriptorBinding& binding) {
            return descriptorFactory.make(binding);
        },
        [this, capability](const CapabilityConfig::ShaderInput& v) {
            return shaderInputFactory.make(capability, v);
        },
        [&, this, resourceId](const CapabilityConfig::PushConstant& pc) {
            registerType(pc.type);
            return pushConstantFactory.make(resourceId, pc);
        },
        [&, this, capability](const CapabilityConfig::RayPayload& pl) {
            registerType(pl.type);
            return rayPayloadFactory.make(capability, pl);
        },
        [this](const CapabilityConfig::HitAttribute& att) {
            return hitAttributeFactory.make(att);
        }
    }, res.resourceType);

    resourceMacros.try_emplace(&res, res.resourceMacroName, accessorStr);
}

} // namespace trc::shader
