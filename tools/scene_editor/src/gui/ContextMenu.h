#pragma once

#include <string>
#include <functional>

#include "viewport/InputViewport.h"

namespace gui
{
    class ContextMenu : public InputViewport
    {
    public:
        ContextMenu(const std::string& title, std::function<void()> drawContents);

        void draw(trc::Frame& frame) override;
        void resize(const ViewportArea& size) override;
        auto getSize() -> ViewportArea override;

        /**
         * @brief Show the global context menu (the 'right-click' menu).
         *
         * Only one context menu can be open at any time.
         */
        static void show(const std::string& title, std::function<void()> drawContents);

        /**
         * @brief Close the global context menu (the 'right-click' menu).
         */
        static void close();

    private:
        static inline s_ptr<ContextMenu> globalContextMenu{ nullptr };

        ViewportArea viewportSize;

        std::string popupTitle;
        std::function<void()> contentsFunc{ []{} };
    };
} // namespace gui
