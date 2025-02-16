#pragma once

#include "Scene.h"
#include "gui/ImguiWindow.h"

namespace gui
{
    class MainMenu;

    class ObjectBrowser : public ImguiWindow
    {
    public:
        explicit ObjectBrowser(s_ptr<Scene> scene);

        void drawWindowContent() override;

        void notifyNewObject(SceneObject obj);
        void notifyRemoveObject(SceneObject obj);

    private:
        s_ptr<Scene> scene;

        SceneObject selected;
    };
} // namespace gui
