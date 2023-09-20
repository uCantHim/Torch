#include "GraphCompiler.h"

#include "Typing.h"



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

        const auto inputNode = graph.socketInfo.get(graph.link.get(sock)).parentNode;
        const auto& buildFunc = graph.nodeInfo.get(inputNode).desc.computation.builder;
        const auto args = buildInputValues(builder, graph, inputNode);
        res.emplace_back(buildFunc(builder, args));
    }

    return res;
}

auto compileMaterialGraph(trc::ShaderModuleBuilder& builder, const GraphTopology& graph)
    -> std::optional<GraphOutput>
{
    if (graph.outputNode == NodeID::NONE) {
        throw std::invalid_argument("[In compileMaterialGraph]: The graph does not have an output"
                                    " node.");
    }

    GraphOutput res;

    const auto& outComp = graph.nodeInfo.get(graph.outputNode).desc.computation;
    auto values = buildInputValues(builder, graph, graph.outputNode, false);
    for (ui32 i = 0; const auto& arg : outComp.arguments) {
        res.values.try_emplace(arg.name, values.at(i++));
    }

    return res;
}
