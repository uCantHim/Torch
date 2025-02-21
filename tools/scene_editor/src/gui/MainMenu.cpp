#include "MainMenu.h"

#include <trc_util/Timer.h>



gui::MainMenu::MainMenu()
    :
    fileExplorer(*this)
{
    openWindow([this]{
        // Show file explorer is separate window
        ig::Begin("File Explorer");
        fileExplorer.drawImGui();
        ig::End();
        return true;
    });
}

void gui::MainMenu::drawImGui()
{
    ig::SetNextWindowBgAlpha(1.0f);
    gui::util::StyleVar style(ImGuiStyleVar_WindowBorderSize, 0);

    // Draw main window content
    if (!ig::Begin("Main Menu")) {
        ig::End();
        return;
    }
    drawMainMenu();
    ig::End();

    // Draw floating windows
    {
        auto range = openWindows.iter();
        for (auto it = range.begin(); it != range.end(); ++it)
        {
            if (!std::invoke(*it)) {
                openWindows.erase(it);
            }
        }
    }
    openWindows.update();
}

void gui::MainMenu::openWindow(std::function<bool()> window)
{
    openWindows.emplace_back(std::move(window));
}

void gui::MainMenu::drawMainMenu()
{
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
}
