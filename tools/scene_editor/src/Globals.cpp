#include "Globals.h"

#include "App.h"



namespace g
{
    auto scene() -> Scene&
    {
        return App::get().getScene();
    }

    auto torch() -> trc::TorchStack&
    {
        return App::get().getTorch();
    }
} // namespace g
