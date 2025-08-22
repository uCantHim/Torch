#include "ImportDialog.h"

#include <ranges>

#include "Globals.h"
#include "ImguiUtil.h"
#include "asset/DefaultAssets.h"



gui::ImportDialog::ImportDialog(const fs::path& path)
    :
    filePath(path)
{
    auto data = trc::importAssets(path);
    if (data) {
        importData = std::move(*data);
    }
    successfulImport = data.has_value();
}

void gui::ImportDialog::drawImGui()
{
    if (!successfulImport)
    {
        ig::Text("Error during import: %s", importError.msg.c_str());
        return;
    }

    ig::Text("Imported %lu meshes from %s", ui64(importData.geometries.size()), filePath.c_str());
    ig::Separator();
    for (const auto& [idx, geo] : importData.geometries | std::views::enumerate)
    {
        // General information
        ig::Text("Imported mesh \"%s\"", geo.name.c_str());
        ig::TreePush(&geo);

        // Vertex information
        ig::Text("%lu vertices", ui64(geo.data.indices.size()));

        // Material information
        auto materials = importData.getMaterials(trc::import::GeoID{ idx });
        ig::Text("%lu materials", ui64(materials.size()));
        if (!materials.empty())
        {
            ig::TreePush(&materials);
            for ([[maybe_unused]] const auto& material : materials)
            {
                ig::Text("A material. More information coming soon.");
            }
            ig::TreePop();
        }

        // Animation information
        if (importData.refs.geoToRig.contains(trc::import::GeoID{ idx }))
        {
            const auto rigId = importData.refs.geoToRig[trc::import::GeoID{ idx }];
            auto& rigData = importData.rigs[rigId].data;
            auto anims = importData.getAnimations(rigId);

            ig::Text("Rig \"%s\"", rigData.name.c_str());
            ig::TreePush(&rigData);
            ig::Text("%lu bones", ui64(rigData.numJoints));
            ig::TreePop();

            ig::Separator();
            ig::Text("%lu animations", ui64(anims.size()));
            for (const auto& anim : anims)
            {
                ig::Text("Animation \"%s\"", anim->name.c_str());
                ig::TreePush(&anim);
                ig::Text("Duration: %fms", anim->data.durationMs);
                ig::Text("%u frames", anim->data.frameCount);
                ig::TreePop();
            }
        }
        else {
            ig::Text("No rigs found");
        }

        if (!imported.contains(geo.name))
        {
            if (ig::Button("Import"))
            {
                g::assets().import(trc::AssetPath(geo.name), geo.data);
                imported.emplace(geo.name);
            }
            if (ig::Button("Import and create in scene"))
            {
                const auto geoId = g::assets().import(trc::AssetPath(geo.name), geo.data);
                if (geoId) {
                    createObject(*geoId, geo.globalTransform);
                    imported.emplace(geo.name);
                }
            }
        }

        ig::TreePop();
    }
}

void gui::ImportDialog::createObject(trc::GeometryID geo, mat4 transform)
{
    auto& scene = g::scene();

    // Create object with default components
    const auto obj = scene.createDefaultObject({ geo, g::mats().undefined });

    auto& d = scene.get<trc::Drawable>(obj);
    d->setFromMatrix(transform);
    if (d->isAnimated()) {
        d->getAnimationEngine()->playAnimation(0);
    }
}
