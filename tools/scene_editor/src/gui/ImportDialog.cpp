#include "ImportDialog.h"

#include "ImguiUtil.h"



gui::FbxImportDialog::FbxImportDialog(const fs::path& filePath)
{
    loadFrom(filePath);
}

void gui::FbxImportDialog::loadFrom(const fs::path& fbxFilePath)
{
    filePath = fbxFilePath;

    trc::FBXLoader loader;
    importData = loader.loadFBXFile(filePath);
}

void gui::FbxImportDialog::drawImGui()
{
    ig::Text("Imported %lu meshes from %s", ui64(importData.meshes.size()), filePath.c_str());
    ig::Separator();
    for (const auto& mesh : importData.meshes)
    {
        // General information
        ig::Text("Imported mesh \"%s\"", mesh.name.c_str());
        ig::TreePush();

        // Vertex information
        ig::Text("%lu vertices", ui64(mesh.geometry.indices.size()));

        // Material information
        ig::Text("%lu materials", ui64(mesh.materials.size()));
        if (!mesh.materials.empty())
        {
            ig::TreePush();
            for (const auto& material : mesh.materials)
            {
                ig::Text("A material. More information coming soon.");
            }
            ig::TreePop();
        }

        // Animation information
        if (mesh.rig.has_value())
        {
            auto& rigData = mesh.rig.value();
            ig::Text("Rig \"%s\"", rigData.name.c_str());
            ig::TreePush();
            ig::Text("%lu bones", ui64(rigData.bones.size()));
            ig::TreePop();

            ig::Separator();
            auto& anims = rigData.animations;
            ig::Text("%lu animations", ui64(anims.size()));
            if (!anims.empty())
            {
                for (const auto& anim : anims)
                {
                    ig::Text("Animation \"%s\"", anim.name.c_str());
                    ig::TreePush();
                    ig::Text("Duration: %fms", anim.durationMs);
                    ig::Text("%u frames", anim.frameCount);
                    ig::TreePop();
                }
            }
        }
        else
        {
            ig::Text("No rigs found");
        }

        ig::TreePop();
    }
}
