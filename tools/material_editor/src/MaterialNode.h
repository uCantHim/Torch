#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <trc/material/BasicType.h>
#include <trc/material/ShaderModuleBuilder.h>

#include "Typing.h"

using namespace trc::basic_types;
namespace code = trc::code;

struct NodeInput
{
    std::string name;
    std::string description;

    TypeConstraint type;
};

struct ConstantValue
{
    std::string name;
    std::string description;

    trc::Constant value;
    SemanticType type;  // TODO (maybe): Make this optional?
};

struct ComputedValue
{
    /**
     * A function that builds the code for an expression.
     *
     * @param trc::ShaderModuleBuilder The code builder used to build the node's
     *                                 expression.
     * @param std::vector<TypedValue>  Arguments to the expression.
     */
    using ComputationBuilder
        = std::function<code::Value(trc::ShaderModuleBuilder&, const std::vector<code::Value>&)>;

    std::string name;
    std::string description;

    /**
     * If this is nullopt, `resultInfluencingArgs` must have at least one entry.
     * Otherwise we can't determine a result type for the computation.
     */
    std::optional<SemanticType> resultType;

    /**
     * A list of arguments from whose types the computation's result type is
     * determined in the following way: `resultType == max(resultInfluencingArgs)`.
     *
     * The computation has an output if the expression
     * `resultType.has_value() || !resultInfluencingArgs.empty()` is true.
     */
    std::vector<ui32> resultInfluencingArgs;

    ComputationBuilder builder;
};

/**
 * A value that output sockets can have.
 */
using NodeOutput = std::variant<ComputedValue, ConstantValue>;

struct NodeDescription
{
    bool hasOutputValue() const {
        return !outputs.empty();
    }

    std::string name;
    std::string id;  // A unique string identifier corresponding to the node's type
    std::string description;

    std::vector<NodeInput> inputs;

    /**
     * A list of pairs of arguments whose types must be equal at build time.
     */
    std::vector<std::pair<ui32, ui32>> inputTypeCorrelations;

    /**
     * Each output receives all inputs
     */
    std::vector<NodeOutput> outputs;
};

auto makeShaderFunctionSignature(const NodeDescription& nodeDesc) -> trc::FunctionType;
auto makeShaderFunction(const NodeDescription& nodeDesc) -> s_ptr<trc::ShaderFunction>;

/**
 * @return A list of all implemented material node types.
 *         Maps [node-id -> node].
 */
auto getMaterialNodes() -> const std::unordered_map<std::string, NodeDescription>&;

/**
 * @return The canonical output node for any material
 */
auto getOutputNode() -> const NodeDescription&;
