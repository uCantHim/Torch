#include "AssetEditor.h"

#include <concepts>

#include <trc/util/TorchDirectories.h>

#include "App.h"
#include "AssetEditorListEntryTrait.h"
#include "ImguiUtil.h"
#include "asset/ImportProcessor.h"



template<typename U> constexpr const char* assetTypeName{ "" };
template<> inline constexpr const char* assetTypeName<trc::Material>{ "Material" };
template<> inline constexpr const char* assetTypeName<trc::Geometry>{ "Geometry" };
template<> inline constexpr const char* assetTypeName<trc::Texture>{ "Texture" };
template<> inline constexpr const char* assetTypeName<trc::Rig>{ "Rig" };
template<> inline constexpr const char* assetTypeName<trc::Animation>{ "Animation" };
template<> inline constexpr const char* assetTypeName<trc::Font>{ "Font" };
template<> inline constexpr const char* assetTypeName<HitboxAsset>{ "Hitbox" };



auto findAvailableName(const trc::AssetManager& assets, trc::AssetPath path) -> trc::AssetPath
{
    if (!assets.exists(path)) return path;

    auto ext = path.filesystemPath("").extension().string();
    auto name = path.replaceExtension("").string();

    size_t i{ 1 };
    do {
        path = trc::AssetPath(name + "(" + std::to_string(i) + ")" + ext);
        ++i;
    } while (assets.exists(path));

    return path;
}



/**
 * @return bool False if the popup has been closed. True while it is open.
 */
template<std::invocable<trc::AssetPath> Func>
bool assetNameInputPopupModal(
    const char* title,
    const trc::AssetManager& dir,
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
                    || trc::Keyboard::isPressed(trc::Key::escape);
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
    assets(app.getAssets())
{
    // I can't define function templates here :'(
#define registerTrait(T) ( \
    assets.registerTrait<AssetEditorListEntryGui, T>( \
        std::make_unique<AssetEditorListEntryGuiImpl<T>>() \
    ) \
    )

    registerTrait(trc::Geometry);
    registerTrait(trc::Texture);
    registerTrait(trc::Material);
    registerTrait(trc::Animation);
    registerTrait(trc::Rig);
    registerTrait(trc::Font);
    registerTrait(HitboxAsset);
}

void gui::AssetEditor::drawImGui()
{
    drawAssetCreateButton();
    drawAssetList();
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
                const auto path = findAvailableName(assets, trc::AssetPath(std::move(name)));
                importAsset(data, path, assets);
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
            ig::Selectable("Geometry", &geoSelected, ImGuiSelectableFlags_DontClosePopups);
            ig::Selectable("Texture",  &texSelected, ImGuiSelectableFlags_DontClosePopups);
        }
        ig::EndPopup();
    }
}

void gui::AssetEditor::drawAssetList()
{
    ig::SetNextItemWidth(ig::GetWindowWidth());

    // Show a material editor for every material that's being edited
    if (ig::BeginListBox("##assets"))
    {
        for (const auto& path : assets.getDataStorage()) {
            drawListEntry(path);
        }
        ig::EndListBox();
    }
}

void gui::AssetEditor::drawListEntry(const trc::AssetPath& path)
{
    assert(assets.exists(path));
    assert(assets.getMetadata(path));

    const auto unique = path.string();
    const auto type = assets.getMetadata(path)->type;
    const auto label = type.getName() + " \"" + unique + "\"";

    ig::Selectable(label.c_str());
    if (ig::BeginPopupContextItem())
    {
        if (auto trait = assets.getTrait<AssetEditorListEntryGui>(type)) {
            trait->drawImGui(assets, path);
        }
        else {
            trc::log::error << trc::log::here() << ": AssetEditorListEntryGui trait is not"
                            << " registered for " << type.getName();
        }

        ig::EndPopup();
    }
    if (ig::IsItemHovered()) {
        ig::SetTooltip("%s", unique.c_str());
    }
}
