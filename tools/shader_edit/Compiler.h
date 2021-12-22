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

        auto compile(CompileConfiguration config) -> CompileResult;

    private:
        auto compileShader(const CompileConfiguration::Meta& meta,
                           ShaderFileConfiguration shader)
            -> std::vector<CompiledShaderFile>;
    };
} // namespace shader_edit
