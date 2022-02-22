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
    constexpr const char* VAR_SEP{ "-" };
    constexpr const char* NAME_TAG_SEP{ ":" };

    std::ifstream file(meta.basePath / shader.inputFilePath);
    if (!file.is_open())
    {
        throw CompileError("Shader file \"" + shader.inputFilePath.string() + "\" does not exist");
    }

    // Parse the document
    std::vector<ShaderDocument> documents{ ShaderDocument(file) };
    std::vector<std::string> uniqueNames{ shader.outputFileName };

    // Create permutations
    for (const auto& [varName, permutations] : shader.variables)
    {
        documents = permutate(documents, varName, permutations);

        // Construct unique names for each permutation
        std::vector<std::string> newNames;
        for (fs::path name : uniqueNames)
        {
            if (permutations.size() == 1) {
                newNames.emplace_back(name);
            }
            else {
                const auto ext = name.extension().string();
                for (const auto& var : permutations)
                {
                    newNames.emplace_back(
                        name.replace_extension(VAR_SEP + varName + NAME_TAG_SEP + var.tag).string()
                        += ext
                    );
                }
            }
        }
        std::swap(uniqueNames, newNames);
    }
    assert(documents.size() == uniqueNames.size());

    // Compile resulting shader files
    std::vector<CompiledShaderFile> result;
    for (uint i = 0; const auto& doc : documents)
    {
        result.emplace_back(CompiledShaderFile{
            .filePath = meta.outDir / fs::path{ uniqueNames[i++] },
            .code     = doc.compile()
        });
    }

    return result;
}
