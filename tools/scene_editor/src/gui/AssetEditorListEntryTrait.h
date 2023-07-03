#pragma once

#include <trc/assets/AssetPath.h>
#include <trc/assets/AssetTraits.h>
#include <trc/assets/Geometry.h>

#include "ImguiUtil.h"
#include "Globals.h"
#include "asset/AssetInventory.h"
#include "asset/DefaultAssets.h"

namespace gui
{
    class AssetEditorListEntryGui : public trc::AssetTrait
    {
    public:
        virtual void drawImGui(AssetInventory& assets, const trc::AssetPath& path) = 0;

        virtual void drawImGui(AssetInventory& assets, trc::AssetID id)
        {
            if (ig::Button("Delete")) {
                assets.manager().destroy(id);
            }
        }

        static void drawDefault(AssetInventory& assets, const trc::AssetPath& path)
        {
            if (ig::Button("Delete")) {
                assets.erase(path);
            }
        }
    };

    template<trc::AssetBaseType T>
    inline void drawAssetEditorListEntryImpl(AssetInventory& assets, const trc::AssetPath& path)
    {
        AssetEditorListEntryGui::drawDefault(assets, path);
    }

    template<trc::AssetBaseType T>
    class AssetEditorListEntryGuiImpl : public AssetEditorListEntryGui
    {
    public:
        /**
         * @brief A default asset menu for unspecialized asset types
         */
        void drawImGui(AssetInventory& assets, const trc::AssetPath& path) override
        {
            drawAssetEditorListEntryImpl<T>(assets, path);
        }
    };

    /**
     * @brief List entry for a geometry
     */
    template<>
    void drawAssetEditorListEntryImpl<trc::Geometry>(
        AssetInventory& assets,
        const trc::AssetPath& path)
    {
        if (ig::Button("Create in scene"))
        {
            static const auto mat = g::mats().undefined;

            auto drawable = g::scene().getDrawableScene().makeDrawable({
                assets.manager().getAs<trc::Geometry>(path).value(),
                mat
            });
            g::scene().createDefaultObject(drawable);
        }

        AssetEditorListEntryGui::drawDefault(assets, path);
    }
} // namespace gui
