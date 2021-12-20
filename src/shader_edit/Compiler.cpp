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
    constexpr const char* VAR_SEP{ "-" };
    constexpr const char* NAME_TAG_SEP{ ":" };

    std::ifstream file(meta.basePath / shader.inputFilePath);
    if (!file.is_open())
    {
        throw CompileError("Shader file \"" + shader.inputFilePath.string() + "\" does not exist");
    }

    std::vector<Document> documents{ Document(file) };
    std::vector<std::string> uniqueNames{ shader.outputFileName };

    // Create permutations
    for (const auto& [varName, permutations] : shader.variables)
    {
        documents = permutate(documents, varName, permutations);

        // Assemble unique file names
        std::vector<std::string> newNames;
        for (const auto& name : uniqueNames)
        {
            for (const auto& var : permutations)
            {
                newNames.emplace_back(name + VAR_SEP + varName + NAME_TAG_SEP + var.tag);
            }
        }
        std::swap(uniqueNames, newNames);
    }
    assert(documents.size() == uniqueNames.size());

    // Compile resulting shader files
    std::vector<CompiledShaderFile> result;
    for (uint i = 0; const auto& doc : documents)
    {
        fs::path name{ uniqueNames[i++] + shader.inputFilePath.extension().string() };
        result.emplace_back(CompiledShaderFile{
            .filePath = meta.outDir / name,
            .code     = doc.compile()
        });
    }

    return result;
}
