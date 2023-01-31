#pragma once

#include "Scene.h"

namespace gui
{
    class MainMenu;

    class ObjectBrowser
    {
    public:
        explicit ObjectBrowser(MainMenu& menu);

        void drawImGui();

        void notifyNewObject(SceneObject obj);
        void notifyRemoveObject(SceneObject obj);

    private:
        App* app;

        SceneObject selected;
    };
} // namespace gui
