#pragma once

#include <concepts>
#include <initializer_list>
#include <utility>

#include <trc/Types.h>
#include <trc/ImguiIntegration.h>
using namespace trc::basic_types;

namespace ig = ImGui;

// End frame and issue return-statement immediately if Begin returns false
#define imGuiTryBegin(title) if (!ImGui::Begin(title)) { ImGui::End(); return; }

// End frame and issue return-statement immediately if Begin returns false
#define imGuiTryBeginReturn(title) if (!ImGui::Begin(title)) { ImGui::End(); return {}; }

namespace gui::util
{
    /**
     * @brief RAII guard for ImGui windows
     *
     * Does *NOT* begin a window!
     */
    struct WindowGuard
    {
        WindowGuard(const WindowGuard&) = delete;
        WindowGuard(WindowGuard&&) noexcept = delete;
        WindowGuard& operator=(const WindowGuard&) = delete;
        WindowGuard& operator=(WindowGuard&&) noexcept = delete;

        WindowGuard() = default;
        ~WindowGuard() noexcept;

        /**
         * @brief Manually close the window
         *
         * Close a window before the guard's stack frame dies. ig::End()
         * will not be called in the destructor if close() has been called
         * first.
         */
        void close();

    private:
        bool closed{ false };
    };

    /**
     * @brief RAII for ImGuiStyleVars
     */
    struct StyleVar
    {
    public:
        StyleVar(const StyleVar&) = delete;
        StyleVar(StyleVar&&) noexcept = delete;
        StyleVar& operator=(const StyleVar&) = delete;
        StyleVar& operator=(StyleVar&&) noexcept = delete;

        explicit StyleVar(ImGuiStyleVar var, float value);
        StyleVar(std::initializer_list<std::pair<ImGuiStyleVar, float>> vars);

        ~StyleVar() noexcept;

    private:
        size_t numStyleVars;
    };

    /**
     * @brief Display a text input field with a confirmation button
     *
     * Takes a callback that is called when the user confirms their input.
     * The buffer can be read during the callback and is being reset after
     * the callback returns.
     *
     * @tparam Func Any callable type
     *
     * @param const char* label    Label of the button
     * @param char*       buf      Buffer for the text input
     * @param size_t      bufSize  Size of the text input buffer
     * @param Func        callback Called when either the button has been
     *                             clicked or the "enter" key has been
     *                             pressed.
     * @param bool clearBufferOnCallback If true, the buffer's contents are
     *                                   set to 0 after the callback has
     *                                   been called.
     */
    template<std::invocable Func>
    inline void textInputWithButton(
        const char* label,
        char* buf,
        size_t bufSize,
        Func callback,
        bool clearBufferOnCallback = true)
    {
        namespace ig = ImGui;

        ig::PushID(label);
        if (ig::InputText("", buf, bufSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
            callback();
            if (clearBufferOnCallback) memset(buf, 0, bufSize);
        }
        ig::SameLine();
        if (ig::Button(label)) {
            callback();
            if (clearBufferOnCallback) memset(buf, 0, bufSize);
        }
        ig::PopID();
    }

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
