#pragma once

#include "GraphManipulator.h"

class MaterialEditorGui
{
public:
    MaterialEditorGui(const trc::Window& window, s_ptr<GraphManipulator> graphManip);

    void drawGui();

private:
    const trc::Window* window;
    s_ptr<GraphManipulator> graph;

    vec2 menuBarSize;
};
