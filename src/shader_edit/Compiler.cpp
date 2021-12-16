#include "shader_edit/Compiler.h"

#include <fstream>

#include "shader_edit/Document.h"
#include "shader_edit/Logger.h"



auto shader_edit::Compiler::compile(CompileConfiguration config) -> CompileResult
{
    CompileResult result;

    for (const auto& shader : config.shaderFiles)
    {
        info("Compiling shader module \"" + shader.filePath.string() + "\"...");
        try {
            for (const auto& compiled : compileShader(std::move(shader)))
            {
                result.shaderFiles.emplace_back(std::move(compiled));
            }
        }
        catch (const CompileError& err) {
            error(err.what());
            continue;
        }
    }

    return result;
}

auto shader_edit::Compiler::compileShader(ShaderFileConfiguration shader)
    -> std::vector<CompiledShaderFile>
{
    std::ifstream file(shader.filePath);
    if (!file.is_open())
    {
        throw CompileError("Shader file \"" + shader.filePath.string() + "\" does not exist");
    }

    Document document{ parseShader(file) };
    for (const auto& [name, var] : shader.variables)
    {
        if (std::holds_alternative<std::string>(var))
        {
            document.set(name, std::move(std::get<std::string>(var)));
        }
        else if (std::holds_alternative<std::vector<std::string>>(var))
        {
            const auto& arr = std::get<std::vector<std::string>>(var);
            document.permutate(name, std::move(arr));
        }
    }

    std::vector<CompiledShaderFile> result;
    for (uint i = 0; const auto& code : document.compile())
    {
        result.emplace_back(CompiledShaderFile{
            .filePath   = std::move(shader.filePath),
            .uniqueName = std::to_string(i++),
            .code       = std::move(code)
        });
    }

    return result;
}
