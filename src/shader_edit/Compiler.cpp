#include "shader_edit/Compiler.h"

#include <fstream>

#include "shader_edit/Document.h"
#include "shader_edit/Logger.h"



auto shader_edit::Compiler::compile(CompileConfiguration config) -> CompileResult
{
    CompileResult result;

    for (auto& shader : config.shaderFiles)
    {
        info("Compiling shader module \"" + shader.inputFilePath.string() + "\"...");
        try {
            auto compileResults = compileShader(config.meta, std::move(shader));
            for (auto& compiled : compileResults)
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

auto shader_edit::Compiler::compileShader(
    const CompileConfiguration::Meta& meta,
    ShaderFileConfiguration shader)
    -> std::vector<CompiledShaderFile>
{
    std::ifstream file(meta.basePath / shader.inputFilePath);
    if (!file.is_open())
    {
        throw CompileError("Shader file \"" + shader.inputFilePath.string() + "\" does not exist");
    }

    std::vector<Document> documents{ Document(file) };
    for (const auto& [name, var] : shader.variables)
    {
        if (std::holds_alternative<std::string>(var))
        {
            for (auto& document : documents) {
                document.set(name, std::move(std::get<std::string>(var)));
            }
        }
        else if (std::holds_alternative<std::vector<std::string>>(var))
        {
            const auto& arr = std::get<std::vector<std::string>>(var);
            documents = permutate(documents, name, std::move(arr));
        }
    }

    std::vector<CompiledShaderFile> result;
    for (uint i = 0; const auto& doc : documents)
    {
        result.emplace_back(CompiledShaderFile{
            .filePath   = meta.outDir / std::move(shader.outputFileName),
            .uniqueName = std::to_string(i++),
            .code       = doc.compile()
        });
    }

    return result;
}
