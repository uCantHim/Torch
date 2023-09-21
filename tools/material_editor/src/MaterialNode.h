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

/**
 * @brief A concrete value generated during graph compilation.
 *
 * Nodes receive these and compile them to a typed output value, considering
 * type constraints.
 */
struct TypedValue
{
    code::Value value;
    trc::BasicType type;
};

/**
 * Description/documentation of an argument
 */
struct ArgDescription
{
    std::string name;
    std::string description;
};

struct NodeComputation
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

    ArgDescription desc;

    /**
     * If this is nullopt, `resultInfluencingArgs` must have at least one entry.
     * Otherwise we can't determine a result type for the computation.
     */
    std::optional<trc::BasicType> resultType;

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

struct NodeDescription
{
    bool hasOutputValue() const {
        return !outputs.empty();
    }

    std::string name;
    std::string id;  // A unique string identifier corresponding to the node's type
    std::string description;

    std::vector<ArgDescription> inputs;
    std::vector<TypeConstraint> inputTypes;

    /**
     * A list of pairs of arguments whose types must be equal at build time.
     */
    std::vector<std::pair<ui32, ui32>> inputTypeCorrelations;

    /**
     * Each output receives all inputs
     */
    std::vector<NodeComputation> outputs;
};

auto makeShaderFunctionSignature(const NodeDescription& nodeDesc) -> trc::FunctionType;
auto makeShaderFunction(const NodeDescription& nodeDesc) -> s_ptr<trc::ShaderFunction>;

auto makeNodeDescription(s_ptr<trc::ShaderFunction> func) -> NodeDescription;

/**
 * @return A list of all implemented material node types.
 *         Maps [node-id -> node].
 */
auto getMaterialNodes() -> const std::unordered_map<std::string, NodeDescription>&;

/**
 * @return The canonical output node for any material
 */
auto getOutputNode() -> const NodeDescription&;
