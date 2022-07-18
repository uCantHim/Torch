#include "AssetEditor.h"

#include <trc/util/TorchDirectories.h>

#include "ProjectDirectory.h"



gui::AssetEditor::AssetEditor(trc::AssetManager& assetManager, ProjectDirectory& assetDir)
    :
    assets(assetManager),
    dir(assetDir)
{
}

void gui::AssetEditor::drawImGui()
{
    imGuiTryBegin("Asset Editor");
    trc::imgui::WindowGuard guard;

    drawMaterialGui();
    drawAssetList();
}

void gui::AssetEditor::drawMaterialGui()
{
    ig::PushItemWidth(200);
    util::textInputWithButton("Add material", matNameBuf.data(), matNameBuf.size(), [this]()
    {
        const std::string matName(matNameBuf.data());
        if (matName.empty()) return;

        try {
            // Create resource on disk
            const trc::AssetPath path(matName);
            dir.save(path, trc::MaterialData{});

            // Create asset
            editedMaterial = assets.create<trc::Material>(path);
            editedMaterialCopy = {}; // New material
        }
        catch (const trc::DuplicateKeyError& err) {
            // Nothing
        }
    });
    ig::PopItemWidth();

    // Draw material editor
    if (editedMaterial)
    {
        materialEditor("Material Editor", editedMaterialCopy,
            [this]() {
                // Material has been saved
                assets.getModule<trc::Material>().modify(
                    editedMaterial.getDeviceID(),
                    [&](auto& mat) { mat = editedMaterialCopy; }
                );

                // Save changes on disk
                dir.save(
                    trc::AssetPath(editedMaterial.getMetaData().uniqueName),
                    editedMaterialCopy
                );

                editedMaterial = trc::MaterialID::NONE;
            },
            [this]() {
                // Don't save; edit has been canceled
                editedMaterial = trc::MaterialID::NONE;
            }
        );
    }
}

void gui::AssetEditor::drawAssetList()
{
    // Show a material editor for every material that's being edited
    if (ig::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        dir.foreach(trc::util::VariantVisitor{
            [this]<trc::AssetBaseType T>(const auto& path){ drawListEntry<T>(path); },
        });
    }
}

template<>
void gui::AssetEditor::drawListEntry<trc::Material>(const trc::AssetPath& path)
{
    const auto id = assets.getAsset<trc::Material>(path);

    ig::PushID(id);
    ig::Text("Material \"%s\"", id.getMetaData().uniqueName.c_str());
    ig::SameLine();
    if (ig::Button("Edit"))
    {
        editedMaterial = id;
        editedMaterialCopy = assets.getModule<trc::Material>().getData(id.getDeviceID());
    }
    ig::PopID();
}

template<>
void gui::AssetEditor::drawListEntry<trc::Geometry>(const trc::AssetPath& path)
{
    ig::Text("Geometry %s", path.getUniquePath().c_str());
}

template<>
void gui::AssetEditor::drawListEntry<trc::Texture>(const trc::AssetPath& path)
{
    ig::Text("Texture %s", path.getUniquePath().c_str());
}

template<>
void gui::AssetEditor::drawListEntry<trc::Rig>(const trc::AssetPath& path)
{
    ig::Text("Rig %s", path.getUniquePath().c_str());
}

template<>
void gui::AssetEditor::drawListEntry<trc::Animation>(const trc::AssetPath& path)
{
    ig::Text("Animation %s", path.getUniquePath().c_str());
}
