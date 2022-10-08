#pragma once

#include <string>
#include <functional>

#include <trc/base/event/InputState.h>
#include <trc/base/event/Event.h>

#include "ImguiUtil.h"

namespace gui
{
    class ContextMenu
    {
    public:
        static void drawImGui();

        static void show(const std::string& title, std::function<void()> drawContents);
        static void close();

    private:
        static inline bool open{ false };

        static inline std::function<void()> contentsFunc{ []{} };
        static inline std::string popupTitle;
        static inline glm::vec2 popupPos;
    };
} // namespace gui
