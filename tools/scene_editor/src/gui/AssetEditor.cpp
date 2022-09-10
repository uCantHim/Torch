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



gui::AssetEditor::AssetEditor(MainMenu& menu)
    :
    app(menu.getApp()),
    mainMenu(menu),
    assets(app.getAssets()),
    dir(app.getProject().getStorageDir())
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
        std::string matName(matNameBuf.data());
        if (matName.empty()) return;

        // Create resource on disk
        const trc::AssetPath path(std::move(matName));
        if (dir.exists(path)) {
            return;
        }

        // Create asset
        auto newMat = assets.create<trc::Material>(path);
        editMaterial(newMat);
    });
    ig::PopItemWidth();

    if (strlen(matNameBuf.data()) && dir.exists(trc::AssetPath(matNameBuf.data())))
    {
        ig::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
        ig::Text("An asset already exists at the path!");
        ig::PopStyleColor();
    }
}

void gui::AssetEditor::editMaterial(trc::MaterialID mat)
{
    mainMenu.openWindow(MaterialEditorWindow(
        mat,
        [this](trc::MaterialID mat, trc::MaterialData data) {
            // Save changes to disk
            dir.save(trc::AssetPath(mat.getMetaData().path.value()), data);
        }
    ));
}

void gui::AssetEditor::drawAssetList()
{
    ig::SetNextItemWidth(ig::GetWindowWidth());

    // Show a material editor for every material that's being edited
    if (ig::BeginListBox("##assets"))
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
        editMaterial(id);
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
