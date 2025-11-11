#pragma once

#include <shader_tools/ShaderDocument.h>

#include "ShaderModule.h"
#include "ShaderModuleBuilder.h"
#include "ShaderOutputInterface.h"

namespace trc::shader
{
    class ShaderModuleCompiler
    {
    public:
        /**
         * @brief Compile a full shader module
         *
         * Compile resource requirements, function definitions, and output value
         * declarations into a shader module.
         *
         * Queries or creates a function "main" and inserts output code
         * (assignments, function calls, ...) into its block.
         */
        static auto compile(const ShaderOutputInterface& output,
                            ShaderModuleBuilder builder)
            -> ShaderModule;
    };
} // namespace trc::shader
