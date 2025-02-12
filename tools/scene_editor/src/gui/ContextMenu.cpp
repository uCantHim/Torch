#include "ContextMenu.h"

#include "gui/ImguiUtil.h"



void gui::ContextMenu::drawImGui()
{
    if (open)
    {
        // Set popup position to mouse cursor
        ig::SetNextWindowPos({ popupPos.x, popupPos.y });
        util::beginContextMenuStyleWindow(popupTitle.c_str());
        gui::util::WindowGuard guard;

        contentsFunc();
    }
}

void gui::ContextMenu::show(const std::string& title, std::function<void()> drawContents)
{
    popupTitle = title;
    popupPos = trc::Mouse::getPosition();
    contentsFunc = std::move(drawContents);
    open = true;
}

void gui::ContextMenu::close()
{
    open = false;
}
