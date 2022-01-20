#include "MainMenu.h"

#include <trc_util/Timer.h>

#include "ImGuiUtil.h"



gui::MainMenu::MainMenu()
{
}

void gui::MainMenu::drawImGui()
{
    drawMainMenu();

    // Draw sub menus
    fileExplorer.drawImGui();
    assetEditor.drawImGui();
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
