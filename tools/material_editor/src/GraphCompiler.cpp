#include "GraphCompiler.h"

#include <functional>

#include "Typing.h"
#include "Util.h"



auto getComputationBuilder(const NodeOutput& value) -> ComputedValue::ComputationBuilder
{
    using Ret = ComputedValue::ComputationBuilder;

    return std::visit(trc::util::VariantVisitor{
        [](const ComputedValue& v) -> Ret { return v.builder; },
        [](const ConstantValue& v) -> Ret {
            return [c=v.value](auto&& builder, auto&&) { return builder.makeConstant(c); };
        }
    }, value);
}

auto inferType(const GraphTopology& graph, SocketID sock) -> std::optional<TypeConstraint>
{
    using namespace std::placeholders;

    const NodeID parentNode = graph.socketInfo.get(sock).parentNode;

    if (auto _out = graph.outputValue.try_get(sock))
    {
        // Socket is an output socket
        if (auto constVal = try_get<ConstantValue>(*_out)) {
            return toShaderType(constVal->type);
        }

        auto& out = std::get<ComputedValue>(*_out);
        if (out.resultType) {
            return TypeRange(*out.resultType);
        }
        if (out.resultInfluencingArgs.empty()) {
            return std::nullopt;
        }

        // Combine input types to infer the output type
        auto inputTypes = graph.inputSockets.get(parentNode)
            | std::views::transform(std::bind(findSocketIndex, std::ref(graph), _1))
            | std::views::filter([&](ui32 i){ return std::ranges::find(out.resultInfluencingArgs, i)
                                                     != out.resultInfluencingArgs.end(); })
            | std::views::transform(std::bind(getIthSocket, std::ref(graph), parentNode, _1))
            | std::views::transform(std::bind(inferType, std::ref(graph), _1));

        std::optional<TypeConstraint> result = inputTypes.front();
        for (auto constr : inputTypes)
        {
            if (!result) break;
            if (!constr) continue;
            result = intersect(*result, *constr);
        }
        return result;
    }

    // Socket is an input socket: it has the type of the socket it is linked
    // to, combined with its own type constraints
    if (auto linked = graph.link.try_get(sock))
    {
        const ui32 socketIndex = findSocketIndex(graph, sock);
        auto baseConstraint = graph.nodeInfo.get(parentNode).desc.inputs.at(socketIndex).type;
        if (auto linkedConstraint = inferType(graph, *linked)) {
            return intersect(baseConstraint, *linkedConstraint);
        }
        return baseConstraint;
    }
    return std::nullopt;
}

/**
 * Up-cast a type to a supertype by filling the supertype's constructor with
 * default values.
 */
auto convertTo(
    code::Value value,
    trc::BasicType srcType,
    trc::BasicType dstType,
    trc::ShaderCodeBuilder& builder)
    -> code::Value
{
    assert(dstType >= srcType && "Should be checked beforehand.");

    // If the source type trivially conforms to the target type, don't perform
    // any cast
    if (srcType == dstType) {
        return value;
    }

    // If only the underlying type differs, perform a cast
    if (srcType.channels == dstType.channels)
    {
        assert(srcType.type < dstType.type);
        return builder.makeCast(dstType, value);
    }

    // If the number of channels differs, fill the larger type's constructor
    // with default values
    const trc::BasicType singleChannel{ dstType.type, 1 };
    const auto defaultValue = builder.makeCast(singleChannel, builder.makeConstant({ 0 }));
    std::vector<code::Value> defaultArgs(dstType.channels - srcType.channels, defaultValue);

    return builder.makeConstructor(dstType, defaultArgs);
}

// Forward declaration for a recursive call in `makeInputSocketValue`.
/**
 * @brief Create the value of an output socket
 */
auto makeOutputSocketValue(
    trc::ShaderModuleBuilder& builder,
    const SocketID sock,
    const GraphTopology& graph) -> std::optional<code::Value>;

/**
 * @brief Create the value of an input socket
 */
auto makeInputSocketValue(
    trc::ShaderModuleBuilder& builder,
    const SocketID sock,
    const GraphTopology& graph) -> std::optional<code::Value>
{
    assert(!graph.outputValue.contains(sock)
           && "Input sockets shall not have associated output values.");

    // First, try to get a value from a linked socket.
    if (auto linkedSocket = graph.link.try_get(sock)) {
        return makeOutputSocketValue(builder, *linkedSocket, graph);
    }

    // Then, try to get a value from a user input field.
    if (auto inputField = graph.socketDecoration.try_get(sock))
    {
        return std::visit(trc::util::VariantVisitor{
            [&](const NumberInputField& f){ return builder.makeConstant(std::stof(f.literalInput)); },
            [&](const ColorInputField& f){ return builder.makeConstant(f.value); },
        }, *inputField);
    }

    // No value is defined for the socket.
    return std::nullopt;
}

auto makeOutputSocketValue(
    trc::ShaderModuleBuilder& builder,
    const SocketID sock,
    const GraphTopology& graph) -> std::optional<code::Value>
{
    // Correctness validation
    if (!graph.outputValue.contains(sock))
    {
        throw GraphValidationError("Input socket is connected to a socket that is not an"
                                   " output socket!");
    }

    const auto node = graph.socketInfo.get(sock).parentNode;

    // Create input values
    std::vector<code::Value> args;
    for (const SocketID inputSock : graph.inputSockets.get(node))
    {
        if (auto value = makeInputSocketValue(builder, inputSock, graph)) {
            args.emplace_back(*value);
        }
        else {
            throw std::runtime_error("Required input socket does not have a value.");
        }
    }

    // Create the output value based on the input values
    const auto& buildFunc = getComputationBuilder(graph.outputValue.get(sock));
    return buildFunc(builder, args);
}

/**
 * @brief Build the values for all input sockets of a node
 */
auto buildInputValues(
    trc::ShaderModuleBuilder& builder,
    const GraphTopology& graph,
    const NodeID node,
    bool requireInputs = true)
    -> std::vector<code::Value>
{
    std::vector<code::Value> res;
    for (const SocketID sock : graph.inputSockets.get(node))
    {
        if (auto value = makeInputSocketValue(builder, sock, graph)) {
            res.emplace_back(*value);
        }
        else if (!requireInputs) {
            res.emplace_back(nullptr);
        }
        else {
            throw std::runtime_error("Required input socket is not linked to any node.");
        }
    }

    return res;
}

auto compileMaterialGraph(trc::ShaderModuleBuilder& builder, const GraphTopology& graph)
    -> std::optional<GraphOutput>
{
    if (graph.outputNode == NodeID::NONE) {
        throw GraphValidationError("[In compileMaterialGraph]: The graph does not have an output"
                                   " node.");
    }

    // Debug logging during development
    auto inputs = graph.inputSockets.get(graph.outputNode)
                  | std::views::transform([&](auto s){ return graph.link.try_get(s); })
                  | std::views::transform([&](auto s){ return s ? inferType(graph, *s) : std::nullopt; });
    for (ui32 i = 0; auto type : inputs)
    {
        if (type)
        {
            if (auto concrete = toConcreteType(*type))
            {
                std::cout << "Input to material parameter #" << i
                          << " has type " << *concrete << "\n";
            }
            else {
                std::cout << "Input to material parameter #" << i << " has type in the range "
                          << "[" << trc::BasicType{ type->underlyingTypeUpperBound, type->minChannels }
                          << ", "
                          << trc::BasicType{ type->underlyingTypeUpperBound, type->maxChannels } << "]"
                          << "\n";
            }
        }
        else {
            std::cout << "Unable to determine input type to material parameter #" << i << "\n";
        }
        ++i;
    }

    // Create all output values of the graph's root node
    GraphOutput res;
    auto values = buildInputValues(builder, graph, graph.outputNode, false);
    assert(values.size() == graph.inputSockets.get(graph.outputNode).size());
    for (ui32 i = 0; const auto& sock : graph.inputSockets.get(graph.outputNode)) {
        res.values.try_emplace(graph.socketInfo.get(sock).name, values.at(i++));
    }

    return res;
}
