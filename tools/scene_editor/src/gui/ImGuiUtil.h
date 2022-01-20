#pragma once

#include <memory>
#include <concepts>
#include <functional>
#include <iostream>

#include <vkb/VulkanBase.h>
#include <vkb/event/Event.h>

#include <trc/Types.h>
#include <trc/experimental/ImguiIntegration.h>
#include <trc/experimental/ImguiUtils.h>
using namespace trc::basic_types;

namespace trc {
    namespace imgui = experimental::imgui;
}
namespace ig = ImGui;

namespace gui::util
{
    using trc::imgui::textInputWithButton;

    /**
     * @brief Display a dropdown selection menu
     *
     * @tparam T Type of the items. Can be deduced.
     *
     * @param const char* label Label for the menu
     * @param const std::vector<std::pair<std::string, T>>& items List of
     *        selectable items. Each entry consists of an item of type T
     *        and a name for the item.
     * @param ui32& selectedIndex Index of the currently selected item.
     *        Will be modified if a different item gets selected.
     *
     * @return True if an element has been selected (i.e. clicked on)
     */
    template<typename T>
    inline bool selectMenu(const char* label,
                           const std::vector<std::pair<std::string, T>>& items,
                           ui32& selectedIndex)
    {
        const char* selectedName = selectedIndex < items.size()
                                   ? items[selectedIndex].first.c_str()
                                   : "";

        bool result = false;
        if (ig::BeginCombo(label, selectedName))
        {
            for (ui32 i = 0; const auto& [name, item] : items)
            {
                if (ig::Selectable(name.c_str(), i == selectedIndex))
                {
                    // An item has been selected
                    selectedIndex = i;
                    result = true;
                }
                i++;
            }
            ig::EndCombo();
        }

        return result;
    }

    inline void beginContextMenuStyleWindow(const char* label)
    {
        // Transparent frame
        ig::PushStyleColor(ImGuiCol_TitleBg, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
        ig::PushStyleColor(ImGuiCol_TitleBgActive, ig::GetStyleColorVec4(ImGuiCol_WindowBg));
        ig::PushStyleColor(ImGuiCol_FrameBg, { 0.0f, 0.0f, 0.0f, 0.0f });

        int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        ig::Begin(label, nullptr, flags);
        ig::PopStyleColor(3);
    }

    class ContextMenu
    {
    public:
        static inline void drawImGui()
        {
            if (!open) return;

            // Set popup position to mouse cursor
            ig::SetNextWindowPos({ popupPos.x, popupPos.y });
            beginContextMenuStyleWindow(popupTitle.c_str());
            trc::imgui::WindowGuard guard;

            contentsFunc();
        }

        static inline void show(const std::string& title, std::function<void()> drawContents)
        {
            popupTitle = "Context \"" + title + "\"";
            popupPos = vkb::Mouse::getPosition();
            contentsFunc = std::move(drawContents);
            open = true;
        }

        static inline void close()
        {
            open = false;
        }

    private:
        static inline bool _init{
            []() {
                vkb::on<vkb::MouseClickEvent>([](auto&&) { close(); });
                return true;
            }()
        };

        static inline bool open{ false };

        static inline std::function<void()> contentsFunc{ []{} };
        static inline std::string popupTitle;
        static inline vec2 popupPos;
    };
} // namespace ig
