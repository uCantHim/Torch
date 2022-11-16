#pragma once

#include <cassert>

#include <string>
#include <vector>

#include "MaterialOutputNode.h"
#include "ShaderCapabilityConfig.h"
#include "ShaderResourceInterface.h"
#include "ShaderModuleBuilder.h"
#include "trc/Types.h"

namespace trc
{
    /**
     * Contains information required to build a material pipeline
     */
    struct MaterialCompileResult
    {
        auto getShaderGlslCode() const -> const std::string&;

        /**
         * @brief Get the name of a material parameter's result variable
         */
        auto getParameterResultVariableName(ParameterID paramNode) const
            -> std::optional<std::string>;

        /**
         * The material compiler always writes a replacement variable at
         * the end of the shader's `main` function:
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

        /**
         * Structs { <texture>, <spec-idx> }
         *
         * The required operation at pipeline creation is:
         *
         *     specConstants[<spec-idx>] = <texture>.getDeviceIndex();
         */
        auto getRequiredTextures() const -> const std::vector<ShaderResources::TextureResource>&;

        auto getRequiredShaderInputs() const
            -> const std::vector<ShaderResources::ShaderInputInfo>&;

        auto getRequiredDescriptorSets() const -> std::vector<std::string>;
        auto getDescriptorIndexPlaceholder(const std::string& setName) const
            -> std::optional<std::string>;
        auto getRequiredPushConstantSize() const -> ui32;

    private:
        friend class MaterialCompiler;

        MaterialCompileResult(
            std::string shaderCode,
            ShaderResources resourceInfo,
            std::unordered_map<ParameterID, std::string> paramResultVariableNames,
            std::string outputReplacementVariableName
        );

        const std::string shaderGlslCode;

        const ShaderResources resources;

        /**
         * Stores the names of temporary variables in which the computed
         * values of material parameters (input nodes to the output node)
         * reside.
         */
        const std::unordered_map<ParameterID, std::string> paramResultVariableNames;

        /** Placeholder variable (in the format `//$...`) after shader outputs */
        const std::string outputReplacementVariableName;
    };

    class MaterialCompiler
    {
    public:
        auto compile(MaterialOutputNode& root, ShaderModuleBuilder& builder)
            -> MaterialCompileResult;
    };
} // namespace trc
