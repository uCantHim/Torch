#include "Compiler.h"

#include <fstream>

#include "Logger.h"



auto shader_edit::Compiler::compile(CompileConfiguration config) -> CompileResult
{
    CompileResult result;

    for (const auto& shader : config.shaderFiles)
    {
        info("Compiling shader file \"" + shader.inputFilePath.string() + "\"...");
        try {
            auto compileResults = compileShader(config.meta, shader);
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
    const ShaderFileConfiguration& shader)
    -> std::vector<CompiledShaderFile>
{
    std::ifstream file(meta.basePath / shader.inputFilePath);
    if (!file.is_open())
    {
        throw CompileError("Shader file \"" + shader.inputFilePath.string() + "\" does not exist");
    }

    // Parse the document
    std::vector<ShaderDocument> documents{ ShaderDocument(file) };
    using Map = std::map<std::string, CompiledShaderFile::VarSpec>;
    std::vector<Map> setVariables{ Map() };

    // Create permutations
    for (const auto& [varName, permutations] : shader.variables)
    {
        documents = permutate(documents, varName, permutations);

        // Store set variable values in a map for each document
        std::vector<Map> newVars;
        for (const Map& map : setVariables)
        {
            for (const auto& [permutationTag, varValue] : permutations)
            {
                newVars.emplace_back(map).try_emplace(varName, permutationTag, varValue.toString());
            }
        }
        std::swap(setVariables, newVars);
    }
    assert(documents.size() == setVariables.size());

    // Collect resulting shader files
    std::vector<CompiledShaderFile> result;
    for (uint i = 0; const auto& doc : documents)
    {
        result.emplace_back(CompiledShaderFile{
            .filePath          = meta.outDir / shader.outputFileName,
            .code              = doc.compile(),
            .variablesToValues = std::move(setVariables[i++]),
        });
    }

    return result;
}
