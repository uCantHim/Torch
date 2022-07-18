#include "ImportDialog.h"

#include "ImguiUtil.h"
#include "App.h"
#include "DefaultAssets.h"
#include "object/Hitbox.h"



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
            ig::Text("%lu animations", ui64(mesh.animations.size()));
            for (const auto& anim : mesh.animations)
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

        if (!imported.contains(mesh.name))
        {
            if (ig::Button("Import")) {
                importGeometry(mesh);
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

auto gui::ImportDialog::importGeometry(const trc::ThirdPartyMeshImport& mesh) -> trc::GeometryID
{
    const trc::AssetPath path(mesh.name);
    app.getProject().getStorageDir().save(path, mesh.geometry);

    const auto geo = app.getAssets().create<trc::Geometry>(path);
    app.addHitbox(geo, makeHitbox(mesh.geometry));

    return geo;
}

void gui::ImportDialog::importAndCreateObject(const trc::ThirdPartyMeshImport& mesh)
{
    auto& scene = app.getScene();

    const auto geoId = importGeometry(mesh);

    // Create object with default components
    const auto obj = scene.createDefaultObject({ geoId, g::mats().undefined });

    auto& d = scene.get<trc::Drawable>(obj);
    d.setFromMatrix(mesh.globalTransform);
    if (d.isAnimated()) {
        d.getAnimationEngine().playAnimation(0);
    }
}
