#include "CMakeDepfileWriter.h"

#include <algorithm>
#include <ranges>

#include "Util.h"



CMakeDepfileWriter::CMakeDepfileWriter(
    const fs::path shaderInputDir,
    const fs::path& targetPath)
    :
    shaderInputDir(shaderInputDir),
    target(targetPath.string())
{
}

void CMakeDepfileWriter::write(const CompileResult& result, std::ostream& os)
{
    if (result.shaders.empty()) return;

    std::vector<std::string> sources;
    for (const auto& [_, shader] : result.shaders)
    {
        auto handleShader = [this, &sources](const ShaderDesc& shader) {
            sources.emplace_back((shaderInputDir / shader.source).string());
        };

        std::visit(VariantVisitor{
            [&](const ShaderDesc& shader){ handleShader(shader); },
            [&](const VariantGroup<ShaderDesc>& group){
                for (const auto& [_, shader] : group.variants) {
                    handleShader(shader);
                }
            },
        }, shader);
    }
    std::ranges::sort(sources);

    os << target << " : ";
    for (const auto& source : sources) {
        os << source << " ";
    }
}

void CMakeDepfileWriter::write(const CompileResult& result, std::ostream& os, std::ostream&)
{
    return write(result, os);
}
