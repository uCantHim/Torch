#pragma once

#include <string>
#include <vector>

#include "BasicType.h"
#include "ShaderCodePrimitives.h"

namespace trc
{
    /**
     * This object is used to build a shader's output interface.
     *
     * Parameters are specified as computed values, which can be linked to
     * output locations.
     */
    class ShaderOutputNode
    {
    public:
        struct ParameterID
        {
            auto operator<=>(const ParameterID&) const = default;
            ui32 index;
        };

        struct OutputID
        {
            auto operator<=>(const OutputID&) const = default;
            ui32 index;
        };

        struct BuiltinOutputID
        {
            auto operator<=>(const BuiltinOutputID&) const = default;
            ui32 index;
        };

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

        ShaderOutputNode() = default;

        /**
         * @brief Create an input parameter
         */
        auto addParameter(BasicType type) -> ParameterID;

        /**
         * @brief Create an attachment output location
         */
        auto addOutput(ui32 location, BasicType type) -> OutputID;

        /**
         * @brief Add an output to a builtin shader variable, e.g. gl_Position
         *
         * An assignment to a variable named `outputName` will be generated
         * if the output has a parameter linked to it.
         *
         * This is technically not restricted to built-in GLSL variables.
         */
        auto addBuiltinOutput(const std::string& outputName) -> BuiltinOutputID;

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
         * The generated code might look like this:
         *
         *     layout (location = 0) vec2 shaderOut;
         *     void main()
         *     {
         *         // ... computes u and v
         *         shaderOut.x = u;
         *         shaderOut.y = v;
         *     }
         *
         * @param ParameterID param  The parameter to link to an output.
         * @param OutputID    output The output to which to link.
         * @param std::string accessor Optional code used to access the
         *        output when assigning the parameter's value. See the
         *        example above for usage.
         *        Can be an empty string.
         */
        void linkOutput(ParameterID param, OutputID output, std::string accessor);
        void linkOutput(ParameterID param, BuiltinOutputID output);

        /**
         * @brief Set a parameter's value
         */
        void setParameter(ParameterID param, code::Value value);

        auto getParameter(ParameterID param) const -> code::Value;
        auto getParameters() const -> const std::vector<code::Value>&;
        auto getOutputLinks() const -> const std::vector<ParameterOutputLink>&;
        auto getOutput(OutputID output) const -> const OutputLocation&;
        auto getOutputs() const -> const std::vector<OutputLocation>&;

        /**
         * A map that maps [variable -> parameter].
         *
         * The generated code is
         *
         *     <variable> = eval(<parameter>);
         */
        auto getBuiltinOutputs() const -> const std::unordered_map<std::string, ParameterID>&;

    private:
        std::vector<code::Value> paramNodes;
        std::vector<BasicType> paramTypes;

        std::vector<ParameterOutputLink> paramOutputLinks;
        std::vector<OutputLocation> outputLocations;

        /** Stores existing outputs added via `addBuiltinOutput` */
        std::vector<std::string> builtinOutputNames;
        /** Links parameters to output names */
        std::unordered_map<std::string, ParameterID> builtinOutputLinks;
    };

    using ParameterID = ShaderOutputNode::ParameterID;
    using OutputID = ShaderOutputNode::OutputID;
} // namespace trc

template<>
struct std::hash<trc::ParameterID>
{
    auto operator()(const trc::ParameterID& id) const -> size_t
    {
        return std::hash<uint32_t>{}(id.index);
    }
};

template<>
struct std::hash<trc::OutputID>
{
    auto operator()(const trc::OutputID& id) const -> size_t
    {
        return std::hash<uint32_t>{}(id.index);
    }
};
