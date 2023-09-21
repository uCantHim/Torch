#include "GraphCompiler.h"

#include <functional>

#include "Typing.h"



auto inferType(const GraphTopology& graph, SocketID sock) -> std::optional<TypeConstraint>
{
    using namespace std::placeholders;

    const NodeID parentNode = graph.socketInfo.get(sock).parentNode;

    if (auto _out = graph.outputComputation.try_get(sock))
    {
        // Socket is an output socket
        auto& out = _out->get();
        if (out.resultType) {
            return TypeRange::makeEq(*out.resultType);
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
        auto baseConstraint = graph.nodeInfo.get(parentNode).desc.inputTypes.at(socketIndex);
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
        if (!graph.link.contains(sock))
        {
            if (requireInputs) {
                throw std::runtime_error("Required input socket is not linked to any node.");
            }
            res.emplace_back(nullptr);
            continue;
        }

        const auto inputSocket = graph.link.get(sock);
        if (!graph.outputComputation.contains(inputSocket))
        {
            throw GraphValidationError("Input socket is connected to a socket that is not an"
                                       " output socket!");
        }

        const auto inputNode = graph.socketInfo.get(inputSocket).parentNode;
        const auto& buildFunc = graph.outputComputation.get(inputSocket).builder;
        const auto args = buildInputValues(builder, graph, inputNode);
        res.emplace_back(buildFunc(builder, args));
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

    GraphOutput res;
    auto values = buildInputValues(builder, graph, graph.outputNode, false);
    for (ui32 i = 0; const auto& sock : graph.inputSockets.get(graph.outputNode)) {
        res.values.try_emplace(graph.socketInfo.get(sock).desc.name, values.at(i++));
    }

    return res;
}
