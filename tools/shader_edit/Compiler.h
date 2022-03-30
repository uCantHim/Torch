#pragma once

#include "CompileConfiguration.h"
#include "ShaderDocument.h"

namespace shader_edit
{
    /**
     * @brief
     */
    class Compiler
    {
    public:
        /**
         * @brief
         */
        Compiler() = default;

        static auto compile(CompileConfiguration config) -> CompileResult;

    private:
        static auto compileShader(const CompileConfiguration::Meta& meta,
                                  const ShaderFileConfiguration& shader)
            -> std::vector<CompiledShaderFile>;
    };
} // namespace shader_edit
