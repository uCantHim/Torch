#include "AssetEditor.h"

#include <concepts>

#include <trc/util/TorchDirectories.h>

#include "ImguiUtil.h"
#include "MaterialEditor.h"
#include "asset/ImportProcessor.h"
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



auto findAvailableName(const ProjectDirectory& dir, trc::AssetPath path) -> trc::AssetPath
{
    if (!dir.exists(path)) return path;

    auto ext = path.getFilesystemPath().extension().string();
    auto name = path.getUniquePath().substr(0, path.getUniquePath().size() - ext.size());

    size_t i{ 1 };
    while (dir.exists(trc::AssetPath(name + "(" + std::to_string(i) + ")" + ext))) ++i;

    return trc::AssetPath(name + "(" + std::to_string(i) + ")" + ext);
}



/**
 * @return bool False if the popup has been closed. True while it is open.
 */
template<std::invocable<trc::AssetPath> Func>
bool assetNameInputPopupModal(
    const char* title,
    const ProjectDirectory& dir,
    Func&& onCreate
    )
{
    ig::OpenPopup(title);
    if (!ig::BeginPopupModal(title)) {
        return true;
    }

    static char assetNameBuf[1024];
    ig::InputText("Name", assetNameBuf, 1024);

    bool validName = strlen(assetNameBuf);

    // Create resource on disk
    static bool overwrite{ false };
    if (validName && dir.exists(trc::AssetPath(assetNameBuf)))
    {
        ig::PushStyleColor(ImGuiCol_Text, 0xFF00A0FF);
        ig::Text("File %s already exists!", assetNameBuf);
        ig::PopStyleColor();

        ig::Checkbox("Overwrite?", &overwrite);
        validName = overwrite;
    }

    if (!validName) ig::BeginDisabled();
    const bool create = ig::Button("Create");
    if (!validName) ig::EndDisabled();
    if (create) {
        onCreate(trc::AssetPath(assetNameBuf));
    }

    ig::SameLine();
    const bool close = create
                    || ig::Button("Cancel")
                    || vkb::Keyboard::isPressed(vkb::Key::escape);
    if (close)
    {
        // Clear static data
        memset(assetNameBuf, 0, 1024);
        overwrite = false;

        // Close popup
        ig::CloseCurrentPopup();
        ig::EndPopup();
        return false;
    }

    ig::EndPopup();
    return true;
}


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
    drawAssetCreateButton();
    drawAssetList();

    for (auto& f : deferredFunctions) {
        f(dir);
    }
    deferredFunctions.clear();
}

void gui::AssetEditor::drawAssetCreateButton()
{
    static bool geoSelected{ false };
    static bool texSelected{ false };

    if (ig::Button("Create Asset")) {
        ig::OpenPopup("##asset_create_selection");
    }
    if (ig::BeginPopup("##asset_create_selection"))
    {
        if (geoSelected || texSelected)
        {
            if (ig::Button("<"))
            {
                geoSelected = false;
                ig::EndPopup();
                return;
            }
            ig::Separator();
        }

        if (geoSelected)
        {
            auto create = [this](std::string name, trc::GeometryData data)
            {
                const auto path = findAvailableName(dir, trc::AssetPath(std::move(name)));
                importAsset(data, path, assets, dir);
            };

            if (ig::Selectable("Cube")) {
                create("cube", trc::makeCubeGeo());
            }
            if (ig::Selectable("Sphere")) {
                create("sphere", trc::makeSphereGeo());
            }
            if (ig::Selectable("Plane")) {
                create("plane", trc::makePlaneGeo());
            }
            ig::Separator();
            if (ig::Selectable("Import")) {}
        }
        else if (texSelected)
        {
            ig::Text("empty");
        }
        else
        {
            if (ig::Selectable("Material"))
            {
                mainMenu.openWindow([this]() -> bool
                {
                    return assetNameInputPopupModal(
                        "Create Material",
                        dir,
                        [this](trc::AssetPath path) {
                            auto newMat = assets.create<trc::Material>(path);
                            dir.save<trc::Material>(path, {}, true);
                            editMaterial(newMat);
                        }
                    );
                });
            }

            ig::Selectable("Geometry", &geoSelected, ImGuiSelectableFlags_DontClosePopups);
            ig::Selectable("Texture",  &texSelected, ImGuiSelectableFlags_DontClosePopups);
        }
        ig::EndPopup();
    }
}

void gui::AssetEditor::editMaterial(trc::MaterialID mat)
{
    mainMenu.openWindow(MaterialEditorWindow(
        mat,
        [this](trc::MaterialID mat, trc::MaterialData data) {
            // Save changes to disk
            dir.save(trc::AssetPath(mat.getMetaData().path.value()), data, true);
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
        ig::SetTooltip(unique.c_str());
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
