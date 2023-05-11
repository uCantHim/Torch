#include "ImportDialog.h"

#include "asset/DefaultAssets.h"
#include "asset/ImportProcessor.h"
#include "object/Hitbox.h"
#include "App.h"
#include "ImguiUtil.h"



gui::ImportDialog::ImportDialog(const fs::path& path, App& app)
    :
    app(app),
    filePath(path),
    importData(trc::loadAssets(path))
{
}

void gui::ImportDialog::drawImGui()
{
    ig::Text("Imported %lu meshes from %s", ui64(importData.meshes.size()), filePath.c_str());
    ig::Separator();
    for (const auto& mesh : importData.meshes)
    {
        // General information
        ig::Text("Imported mesh \"%s\"", mesh.name.c_str());
        ig::TreePush(&mesh);

        // Vertex information
        ig::Text("%lu vertices", ui64(mesh.geometry.indices.size()));

        // Material information
        ig::Text("%lu materials", ui64(mesh.materials.size()));
        if (!mesh.materials.empty())
        {
            ig::TreePush(&mesh.materials);
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
            ig::TreePush(&rigData);
            ig::Text("%lu bones", ui64(rigData.bones.size()));
            ig::TreePop();

            ig::Separator();
            ig::Text("%lu animations", ui64(mesh.animations.size()));
            for (const auto& anim : mesh.animations)
            {
                ig::Text("Animation \"%s\"", anim.name.c_str());
                ig::TreePush(&anim);
                ig::Text("Duration: %fms", anim.durationMs);
                ig::Text("%u frames", anim.frameCount);
                ig::TreePop();
            }
        }
        else {
            ig::Text("No rigs found");
        }

        if (!imported.contains(mesh.name))
        {
            if (ig::Button("Import"))
            {
                importAsset(mesh.geometry, trc::AssetPath(mesh.name), app.getAssets());
                imported.emplace(mesh.name);
            }
            if (ig::Button("Import and create in scene")) {
                importAndCreateObject(mesh);
                imported.emplace(mesh.name);
            }
        }

        ig::TreePop();
    }
}

void gui::ImportDialog::importAndCreateObject(const trc::ThirdPartyMeshImport& mesh)
{
    auto& scene = app.getScene();

    const auto geoId = importAsset(mesh.geometry, trc::AssetPath(mesh.name), app.getAssets());

    // Create object with default components
    const auto obj = scene.createDefaultObject({ geoId, g::mats().undefined });

    auto& d = scene.get<trc::Drawable>(obj);
    d->setFromMatrix(mesh.globalTransform);
    if (d->isAnimated()) {
        d->getAnimationEngine().value()->playAnimation(0);
    }
}
