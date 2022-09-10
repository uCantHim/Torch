#include "AssetEditor.h"

#include <trc/util/TorchDirectories.h>

#include "ImguiUtil.h"
#include "MaterialEditor.h"
#include "asset/ProjectDirectory.h"
#include "App.h"



template<typename U> constexpr const char* assetTypeName{ "" };
template<> inline constexpr const char* assetTypeName<trc::Material>{ "Material" };
template<> inline constexpr const char* assetTypeName<trc::Geometry>{ "Geometry" };
template<> inline constexpr const char* assetTypeName<trc::Texture>{ "Texture" };
template<> inline constexpr const char* assetTypeName<trc::Rig>{ "Rig" };
template<> inline constexpr const char* assetTypeName<trc::Animation>{ "Animation" };
template<> inline constexpr const char* assetTypeName<trc::Font>{ "Font" };
template<> inline constexpr const char* assetTypeName<HitboxAsset>{ "Hitbox" };



gui::AssetEditor::AssetEditor(App& _app)
    :
    app(_app),
    assets(_app.getAssets()),
    dir(_app.getProject().getStorageDir())
{
}

void gui::AssetEditor::drawImGui()
{
    drawMaterialGui();
    drawAssetList();

    for (auto& f : deferredFunctions) {
        f(dir);
    }
    deferredFunctions.clear();
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
                    trc::AssetPath(editedMaterial.getMetaData().name),
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
    if (ig::BeginListBox("Assets"))
    {
        dir.foreach(trc::util::VariantVisitor{
            [this]<trc::AssetBaseType T>(const auto& path){ drawListEntry<T>(path); },
        });
        ig::EndListBox();
    }
}

template<trc::AssetBaseType T>
void gui::AssetEditor::drawListEntry(const trc::AssetPath& path)
{
    const auto unique = path.getUniquePath();
    const auto label = assetTypeName<T> + std::string(" \"" + unique + "\"");

    ig::Selectable(label.c_str());
    if (ig::BeginPopupContextItem())
    {
        drawEntryContextMenu<T>(path);
        ig::EndPopup();
    }
    if (ig::IsItemHovered()) {
        ig::SetTooltip("%s", unique.c_str());
    }
}

template<trc::AssetBaseType T>
void gui::AssetEditor::drawEntryContextMenu(const trc::AssetPath& path)
{
    drawDefaultEntryContext(path);
}

template<>
void gui::AssetEditor::drawEntryContextMenu<trc::Geometry>(const trc::AssetPath& path)
{
    if (ig::Button("Create in scene"))
    {
        const auto mat = assets.create(trc::MaterialData{ .color=vec4(1.0f) });
        auto obj = app.getScene().createObject();
        app.getScene().add<trc::Drawable>(obj, trc::Drawable(
            assets.get<trc::Geometry>(path),
            mat,
            app.getScene().getDrawableScene()
        ));
    }
    drawDefaultEntryContext(path);
}

template<>
void gui::AssetEditor::drawEntryContextMenu<trc::Material>(const trc::AssetPath& path)
{
    if (ig::Button("Edit"))
    {
        const auto id = assets.get<trc::Material>(path);
        editedMaterial = id;
        editedMaterialCopy = assets.getModule<trc::Material>().getData(id.getDeviceID());
    }
    drawDefaultEntryContext(path);
}

void gui::AssetEditor::drawDefaultEntryContext(const trc::AssetPath& path)
{
    if (ig::Button("Delete"))
    {
        assets.destroy(path);
        defer([path=path](auto& dir){ dir.remove(path); });
    }
}

void gui::AssetEditor::defer(std::function<void(ProjectDirectory&)> func)
{
    deferredFunctions.emplace_back(std::move(func));
}
