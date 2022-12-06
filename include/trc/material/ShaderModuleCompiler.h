#pragma once

#include <cassert>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShaderOutputNode.h"
#include "ShaderResourceInterface.h"
#include "ShaderModuleBuilder.h"
#include "trc/Types.h"

namespace trc
{
    /**
     * Holds information about a compiled shader module.
     */
    struct ShaderModule : ShaderResources
    {
        auto getGlslCode() const -> const std::string&;

        /**
         * @brief Get the name of an output parameter's result variable
         *
         * Parameters specified at the module's ShaderOutputNode are
         * computed in the shader. This method retrieves a GLSL variable
         * name that refers to this final value before it is written to a
         * shader output location.
         */
        auto getParameterName(ParameterID paramNode) const
            -> std::optional<std::string>;

    private:
        friend class ShaderModuleCompiler;

        ShaderModule(
            std::string shaderCode,
            ShaderResources resourceInfo,
            std::unordered_map<ParameterID, std::string> paramResultVariableNames
        );

        const std::string shaderGlslCode;

        /**
         * Stores the names of variables in which the computed values of
         * output parameters (inputs to the output node) reside.
         */
        const std::unordered_map<ParameterID, std::string> paramResultVariableNames;
    };

    class ShaderModuleCompiler
    {
    public:
        auto compile(ShaderOutputNode& output, ShaderModuleBuilder& builder)
            -> ShaderModule;
    };
} // namespace trc
