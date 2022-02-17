#include "ImportDialog.h"

#include <trc/asset_import/AssetUtils.h>

#include "ImguiUtil.h"
#include "App.h"
#include "DefaultAssets.h"



gui::ImportDialog::ImportDialog(const fs::path& filePath)
{
    loadFrom(filePath);
}

void gui::ImportDialog::loadFrom(const fs::path& fbxFilePath)
{
    filePath = fbxFilePath;
    importData = trc::loadGeometry(fbxFilePath);
}

void gui::ImportDialog::drawImGui()
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
            for ([[maybe_unused]] const auto& material : mesh.materials)
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
            for (const auto& anim : anims)
            {
                ig::Text("Animation \"%s\"", anim.name.c_str());
                ig::TreePush();
                ig::Text("Duration: %fms", anim.durationMs);
                ig::Text("%u frames", anim.frameCount);
                ig::TreePop();
            }
        }
        else {
            ig::Text("No rigs found");
        }

        if (ig::Button("Import")) {
            importGeometry(mesh);
        }
        if (ig::Button("Import and create in scene")) {
            importAndCreateObject(mesh);
        }

        ig::TreePop();
    }
}

auto gui::ImportDialog::importGeometry(const trc::Mesh& mesh) -> trc::GeometryID
{
    auto& am = App::get().getAssets();
    return am.add(mesh.geometry, mesh.rig);
}

void gui::ImportDialog::importAndCreateObject(const trc::Mesh& mesh)
{
    auto& scene = App::get().getScene();

    const auto geoId = importGeometry(mesh);
    const auto obj = scene.createDefaultObject({ geoId, g::mats().undefined });

    auto& d = scene.get<trc::Drawable>(obj);
    d.setFromMatrix(mesh.globalTransform);
    if (mesh.rig.has_value()) {
        d.getAnimationEngine().playAnimation(0);
    }
}
