#include "GraphInteraction.h"

#include "Util.h"



void createInputFields(GraphInteraction& inter, NodeID node, const GraphTopology& graph)
{
    for (const auto& sock : graph.outputSockets.get(node))
    {
        if (graph.socketDecoration.contains(sock))
        {
            std::visit(trc::util::VariantVisitor{
                [&](const NumberInputField& f){
                    inter.textInputFields.emplace(sock, std::to_string(f.value));
                },
                [&](const ColorInputField& f) {
                    inter.colorInputFields.emplace(sock, f.value);
                },
            }, graph.socketDecoration.get(sock));
        }
    }
}

void updateOutputValues(const GraphInteraction& in, GraphTopology& graph)
{
    for (const auto& [sock, str] : in.textInputFields.items())
    {
        try {
            const float val = std::stof(str);
            if (auto constVal = try_get<ConstantValue>(graph.outputValue.get(sock))) {
                constVal->value = val;
            }
            else {
                std::cout << "Warning: Socket with an input field decoration (current value: "
                          << val << ") does not have a corresponding output value entry.";
            }
        }
        catch (const std::invalid_argument&) {
            // The text input was not parseable as a float.
            // Don't update the value.
        }
    }

    for (const auto& [sock, color] : in.colorInputFields.items())
    {
        if (auto constVal = try_get<ConstantValue>(graph.outputValue.get(sock))) {
            constVal->value = color;
        }
        else {
            std::cout << "Warning: Socket with a color input field decoration"
                         " does not have a corresponding output value entry.";
        }
    }
}
