#pragma once

#include <cassert>

#include <string>
#include <vector>

#include "MaterialGraph.h"
#include "MaterialOutputNode.h"
#include "ShaderCapabilityConfig.h"
#include "ShaderResourceInterface.h"
#include "trc/Types.h"

namespace trc
{
    /**
     * Contains information required to build a material pipeline
     */
    struct MaterialCompileResult
    {
        /**
         * @brief Get the name of a material parameter's result variable
         */
        auto getParameterResultVariableName(ParameterID paramNode) const
            -> std::optional<std::string>;

        /**
         * The material compiler always writes a replacement variable at
         * the end of the fragment shader's `main` function:
         *
         *     void main()
         *     {
         *         ...
         *
         *         //$ POST_OUTPUT_VARIABLE
         *     }
         *
         * This function queries this variable's name for post-processing
         * purposes. (In the example above, this function would return the
         * string "POST_OUTPUT_VARIABLE")
         */
        auto getOutputPlaceholderVariableName() const -> std::string;

        const std::string fragmentGlslCode;

        /**
         * Structs { <texture>, <spec-idx> }
         *
         * The required operation at pipeline creation is:
         *
         *     specConstants[<spec-idx>] = <texture>.getDeviceIndex();
         */
        std::vector<ShaderResources::TextureResource> requiredTextures;

    private:
        friend class MaterialCompiler;

        MaterialCompileResult(
            std::string fragmentCode,
            std::vector<ShaderResources::TextureResource> specConstTextures,
            std::unordered_map<ParameterID, std::string> paramResultVariableNames,
            std::string outputReplacementVariableName
        );

        /**
         * Stores the names of temporary variables in which the computed
         * values of material parameters (input nodes to the output node)
         * reside.
         */
        const std::unordered_map<ParameterID, std::string> paramResultVariableNames;

        /** Placeholder variable (in the format `//$...`) after fragment outputs */
        const std::string outputReplacementVariableName;
    };

    class MaterialCompiler
    {
    public:
        explicit MaterialCompiler(ShaderCapabilityConfig config);

        auto compile(MaterialOutputNode& root) -> MaterialCompileResult;

    private:
        static auto compileFunctions(ShaderResourceInterface& resources, MaterialOutputNode& mat)
            -> std::string;

        static auto call(MaterialNode* node) -> std::string;

        ShaderCapabilityConfig config;
    };
} // namespace trc
