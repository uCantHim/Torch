#include "MainMenu.h"

#include <trc_util/Timer.h>

#include "App.h"



gui::MainMenu::MainMenu(App& app)
    :
    assetEditor(app.getAssets())
{
}

void gui::MainMenu::drawImGui()
{
    drawMainMenu();

    // Draw sub menus
    fileExplorer.drawImGui();
    assetEditor.drawImGui();

    for (auto it = openWindows.begin(); it != openWindows.end(); /* nothing */)
    {
        if (!std::invoke(*it)) {
            it = openWindows.erase(it);
        }
        else {
            ++it;
        }
    }
}

void gui::MainMenu::openWindow(std::function<bool()> window)
{
    openWindows.emplace_back(std::move(window));
}

void gui::MainMenu::drawMainMenu()
{
    imGuiTryBegin("Main Menu");

    static int frames{ 0 };
    static int framesPerSecond{ 0 };
    static trc::Timer timer;

    // Count FPS
    frames++;
    if (timer.duration() > 1000)
    {
        timer.reset();
        framesPerSecond = frames;
        frames = 0;
    }
    ig::Text("%i FPS", framesPerSecond);

    ig::End();
}
