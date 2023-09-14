#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <trc/material/BasicType.h>
#include <trc/material/ShaderModuleBuilder.h>

using namespace trc::basic_types;
namespace code = trc::code;

struct NodeValue
{
    trc::BasicType type;

    std::string name;
    std::string description;
};

using NodeComputationBuilder
    = std::function<code::Value(trc::ShaderModuleBuilder&, std::vector<code::Value>)>;

struct NodeDescription
{
    std::string name;
    std::string technicalName;
    std::string description;
    NodeComputationBuilder computation;

    std::vector<NodeValue> inputs;
    std::optional<NodeValue> output;
};

auto makeShaderFunctionSignature(const NodeDescription& nodeDesc) -> trc::FunctionType;
auto makeShaderFunction(const NodeDescription& nodeDesc) -> s_ptr<trc::ShaderFunction>;

auto makeNodeDescription(s_ptr<trc::ShaderFunction> func) -> NodeDescription;
