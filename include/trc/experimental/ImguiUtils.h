#pragma once

#include "ImguiIntegration.h"

// End frame and issue return-statement immediately if Begin returns false
#define imGuiTryBegin(title) if (!ImGui::Begin(title)) { ImGui::End(); return; }

// End frame and issue return-statement immediately if Begin returns false
#define imGuiTryBeginReturn(title) if (!ImGui::Begin(title)) { ImGui::End(); return {}; }

namespace trc::experimental::imgui
{
    struct WindowGuard
    {
        std::unique_ptr<int, std::function<void(int*)>> guard{
            &dummy,
            [](int*) { ig::End(); }
        };

    private:
        static inline int dummy{ 0 };
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
} // namespace trc::experimental::imgui
