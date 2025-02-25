#include "Globals.h"

#include "App.h"



namespace g
{
    auto assets() -> AssetInventory&
    {
        return App::get().getAssets();
    }

    auto scene() -> Scene&
    {
        return App::get().getScene();
    }

    auto torch() -> trc::TorchStack&
    {
        return App::get().getTorch();
    }

    void openFloatingViewport(s_ptr<Viewport> vp)
    {
        App::get().getViewportManager().createFloating(std::move(vp));
    }

    void closeFloatingViewport(Viewport* vp)
    {
        if (vp != nullptr) {
            App::get().getViewportManager().remove(vp);
        }
    }
} // namespace g
