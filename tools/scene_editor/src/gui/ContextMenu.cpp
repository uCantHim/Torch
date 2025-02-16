#include "ContextMenu.h"

#include "Globals.h"
#include "gui/ImguiUtil.h"



gui::ContextMenu::ContextMenu(const std::string& title, std::function<void()> drawContents)
    :
    popupTitle(title),
    contentsFunc(std::move(drawContents))
{
}

void gui::ContextMenu::draw(trc::Frame&)
{
    const auto [pos, _] = getSize();

    ig::SetNextWindowPos({ static_cast<float>(pos.x), static_cast<float>(pos.y) });
    util::beginContextMenuStyleWindow(popupTitle.c_str());
    contentsFunc();
    ig::End();
}

void gui::ContextMenu::resize(const ViewportArea& size)
{
    viewportSize = size;
}

auto gui::ContextMenu::getSize() -> ViewportArea
{
    return viewportSize;
}

void gui::ContextMenu::show(const std::string& title, std::function<void()> drawContents)
{
    close();

    if (globalContextMenu == nullptr) {
        globalContextMenu = std::make_shared<ContextMenu>(title, []{});
    }

    globalContextMenu->popupTitle = title;
    globalContextMenu->contentsFunc = std::move(drawContents);
    globalContextMenu->resize({ { 400, 100 }, {} });
    g::openFloatingViewport(globalContextMenu);
}

void gui::ContextMenu::close()
{
    g::closeFloatingViewport(globalContextMenu.get());
}
