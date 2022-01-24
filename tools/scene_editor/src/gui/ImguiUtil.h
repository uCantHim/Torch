#pragma once

#include <trc/Types.h>
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

    /**
     * @brief Begin a window in the style of a context menu
     */
    void beginContextMenuStyleWindow(const char* label);
} // namespace gui::util
