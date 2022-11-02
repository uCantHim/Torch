#pragma once

#include <string>
#include <vector>

#include "BasicType.h"

namespace trc
{
    class MaterialNode;

    /**
     * This object is used to build a fragment shader output interface.
     *
     * Parameters are inputs to the output node. Values for parameters
     * are specified as MaterialNodes. This makes the material output node
     * the root of the material graph.
     *
     * Outputs specify pipeline attachment locations to which parameter
     * values can be written.
     *
     * Parameters can be linked to outputs.
     */
    class MaterialOutputNode
    {
    public:
        using ParameterID = ui32;
        using OutputID = ui32;

        struct ParameterOutputLink
        {
            ParameterID param;
            OutputID output;
            std::string outputAccessor;
        };

        struct OutputLocation
        {
            ui32 location;
            BasicType type;
        };

        MaterialOutputNode() = default;

        /**
         * @brief Create an input parameter
         */
        auto addParameter(BasicType type) -> ParameterID;

        /**
         * @brief Create an attachment output location
         */
        auto addOutput(ui32 location, BasicType type) -> OutputID;

        /**
         * @brief Link a parameter to an output location
         *
         * Multiple parameters can be linked to the same output location.
         * For example:
         *
         *     auto u = node.addParameter(float{});
         *     auto v = node.addParameter(float{});
         *     auto outUV = node.addOutput(0, vec2{});
         *
         *     node.linkOutput(u, outUV, ".x");
         *     node.linkOutput(v, outUV, ".y");
         *
         * @param ParameterID param  The parameter to link to an output.
         * @param OutputID    output The output to which to link.
         * @param std::string accessor Optional code used to access the
         *        output when assigning the parameter's value. See the
         *        example above for usage.
         *        Can be an empty string.
         */
        void linkOutput(ParameterID param, OutputID output, std::string accessor);

        /**
         * @brief Set a parameter's value
         */
        void setParameter(ParameterID param, MaterialNode* value);

        auto getParameter(ParameterID param) const -> MaterialNode*;
        auto getParameters() const -> const std::vector<MaterialNode*>&;
        auto getOutputLinks() const -> const std::vector<ParameterOutputLink>&;
        auto getOutput(OutputID output) const -> const OutputLocation&;
        auto getOutputs() const -> const std::vector<OutputLocation>&;

    private:
        std::vector<MaterialNode*> paramNodes;
        std::vector<BasicType> paramTypes;

        std::vector<ParameterOutputLink> paramOutputLinks;
        std::vector<OutputLocation> outputLocations;
    };

    using ParameterID = MaterialOutputNode::ParameterID;
    using OutputID = MaterialOutputNode::OutputID;
} // namespace trc
