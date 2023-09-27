#pragma once

#include <trc/base/event/Event.h>
#include <trc/core/Window.h>

#include "MaterialEditorCommands.h"

class MaterialEditorGui
{
public:
    MaterialEditorGui(const trc::Window& window, MaterialEditorCommands& commands);

    void drawGui();

    void openContextMenu(vec2 position);
    void closeContextMenu();

    /**
     * @brief imgui-style function that draws an input popup
     *
     * @return optional<float> The value entered by the user if an input has
     *                         occurred, otherwise nothing.
     */
    auto inputPopupNumber() -> std::optional<float>;

    auto inputPopupColor() -> std::optional<float>;

private:
    static constexpr float kContextMenuAlpha{ 0.85f };

    void drawMainMenuContents();

    trc::UniqueListenerId<trc::SwapchainResizeEvent> onResize;

    MaterialEditorCommands& graph;

    vec2 menuBarSize;

    vec2 contextMenuPos{ 0.0f, 0.0f };
    bool contextMenuIsOpen{ false };
};
