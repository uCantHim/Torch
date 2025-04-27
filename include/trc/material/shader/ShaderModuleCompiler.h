#pragma once

#include <cassert>

#include <string>

#include <shader_tools/ShaderDocument.h>

#include "ShaderModuleBuilder.h"
#include "ShaderOutputInterface.h"
#include "ShaderResourceInterface.h"

namespace trc::shader
{
    /**
     * Holds information about a compiled shader module.
     */
    struct ShaderModule : ShaderResourceInterface
    {
        ShaderModule(const ShaderModule&) = default;
        ShaderModule(ShaderModule&&) = default;
        ShaderModule& operator=(const ShaderModule&) noexcept = default;
        ShaderModule& operator=(ShaderModule&&) noexcept = default;
        ~ShaderModule() noexcept = default;

        /**
         * @brief Create a shader module.
         */
        ShaderModule(shader_edit::ShaderDocument shaderCode,
                     ShaderResourceInterface resourceInfo);

        /**
         * @return The code for the entire shader module. May contain unset
         *         variables, such as descriptor set index placeholders. The
         *         shader module has functions to query information about these.
         */
        auto getShaderCode() -> shader_edit::ShaderDocument&;

        /**
         * @return The code for the entire shader module. May contain unset
         *         variables, such as descriptor set index placeholders. The
         *         shader module has functions to query information about these.
         */
        auto getShaderCode() const -> const shader_edit::ShaderDocument&;

    private:
        // Hide this so it cannot be confused with the module's `getShaderCode`.
        using ShaderResourceInterface::getGlslCode;

        shader_edit::ShaderDocument shaderCode;
    };

    class ShaderModuleCompiler
    {
    public:
        /**
         * @brief Compile a full shader module
         *
         * Compile resource requirements, function definitions, and output value
         * declarations into a shader module.
         *
         * Queries or creates a function "main" and appends output code
         * (assignments, function calls, ...) to it's block.
         */
        static auto compile(const ShaderOutputInterface& output,
                            ShaderModuleBuilder builder,
                            const CapabilityConfig& caps)
            -> ShaderModule;
    };
} // namespace trc::shader
