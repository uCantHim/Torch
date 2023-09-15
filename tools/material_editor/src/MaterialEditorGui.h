#pragma once

#include <trc/base/event/Event.h>
#include <trc/core/Window.h>

#include "GraphManipulator.h"

class MaterialEditorGui
{
public:
    MaterialEditorGui(const trc::Window& window, s_ptr<GraphManipulator> graphManip);

    void drawGui();

    void openContextMenu(vec2 position);
    void closeContextMenu();

private:
    static constexpr float kContextMenuAlpha{ 0.85f };

    void drawMainMenuContents();

    trc::UniqueListenerId<trc::SwapchainResizeEvent> onResize;

    s_ptr<GraphManipulator> graph;

    vec2 menuBarSize;

    vec2 contextMenuPos{ 0.0f, 0.0f };
    bool contextMenuIsOpen{ false };
};
