#include "MainMenu.h"

#include <trc_util/Timer.h>

#include "App.h"



gui::MainMenu::MainMenu(App& app)
    :
    app(app),
    fileExplorer(*this),
    assetEditor(app)
{
}

void gui::MainMenu::drawImGui()
{
    const vec2 windowSize = app.getTorch().getWindow().getWindowSize();
    ig::SetNextWindowPos({ -1, -1 });
    ig::SetNextWindowSize({ windowSize.x * 0.25f, windowSize.y + 2 });
    ig::SetNextWindowBgAlpha(1.0f);

    int flags = ImGuiWindowFlags_NoResize
              | ImGuiWindowFlags_NoTitleBar
              | ImGuiWindowFlags_NoBringToFrontOnFocus;
    trc::imgui::WindowGuard guard;
    if (!ig::Begin("Main Menu", nullptr, flags)) {
        return;
    }

    drawMainMenu();

    // Draw sub menus
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

    // Show file explorer is separate window
    guard.close();
    ig::Begin("File Explorer");
    fileExplorer.drawImGui();
    ig::End();
}

auto gui::MainMenu::getApp() -> App&
{
    return app;
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
