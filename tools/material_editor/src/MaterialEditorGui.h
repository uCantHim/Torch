#pragma once

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

    const trc::Window* window;
    s_ptr<GraphManipulator> graph;

    vec2 menuBarSize;

    vec2 contextMenuPos{ 0.0f, 0.0f };
    bool contextMenuIsOpen{ false };
};
