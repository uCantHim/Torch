#include "MaterialEditorCommands.h"

#include <trc/material/FragmentShader.h>

#include "GraphCompiler.h"
#include "GraphSerializer.h"
#include "MaterialPreview.h"



MaterialEditorCommands::MaterialEditorCommands(
    GraphScene& graph,
    MaterialPreview& preview)
    :
    GraphManipulator(graph),
    graph(graph),
    previewWindow(preview)
{
}

void MaterialEditorCommands::saveGraphToCurrentFile()
{
    saveGraphToFile(".matedit_save");
}

void MaterialEditorCommands::saveGraphToFile(const fs::path& path)
{
    std::ofstream file(path, std::ios::binary);
    serializeGraph(graph, file);
    std::cout << "Saved.\n";
}

void MaterialEditorCommands::compileMaterial()
{
    const std::array<trc::Constant, 6> defaultValues{{
        vec3(1.0f),     // Color
        vec3(0, 1, 0),  // Normal
        0.1f,           // Specular factor
        0.5f,           // Roughness
        0.0f,
        true
    }};

    trc::ShaderModuleBuilder builder;
    auto output = compileMaterialGraph(builder, graph.graph);
    if (!output)
    {
        std::cout << "Error during material compilation.\n";
        return;
    }

    using P = trc::FragmentModule::Parameter;
    trc::FragmentModule frag;
    auto trySet = [&](P param, const char* name) {
        auto it = output->values.find(name);
        if (it != output->values.end() && it->second != nullptr) {
            frag.setParameter(param, it->second);
        }
        else {
            const size_t index = static_cast<size_t>(param);
            frag.setParameter(param, builder.makeConstant(defaultValues[index]));
        }
    };

    trySet(P::eColor, "Albedo");
    trySet(P::eEmissive, "Emissive");
    trySet(P::eMetallicness, "Metallicness");
    trySet(P::eNormal, "Normal");
    trySet(P::eRoughness, "Roughness");
    trySet(P::eSpecularFactor, "Specular Factor (shinyness)");

    trc::MaterialData mat{ {frag.build(builder, false), false} };
    previewWindow.makeMaterial(std::move(mat));
}
