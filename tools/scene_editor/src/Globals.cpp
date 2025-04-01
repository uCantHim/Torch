#include "Globals.h"

#include "App.h"
#include "input/InputProcessor.h"



namespace g
{
    auto keyboard() -> const KeyboardState&
    {
        return InputProcessor::getGlobalInputState().keyboard;
    }

    auto mouse() -> const MouseState&
    {
        return InputProcessor::getGlobalInputState().mouse;
    }

    auto assets() -> AssetInventory&
    {
        return App::get().getAssets();
    }

    auto scene() -> Scene&
    {
        return App::get().getScene();
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
