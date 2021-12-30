#pragma once

#include <GLFW/glfw3.h>

#include "../VulkanInclude.h"

namespace vkb
{
    /**
     * These have the same numeric value as the corresponding GLFW defines.
     */
    enum class Key {
        a = GLFW_KEY_A,
        b = GLFW_KEY_B,
        c = GLFW_KEY_C,
        d = GLFW_KEY_D,
        e = GLFW_KEY_E,
        f = GLFW_KEY_F,
        g = GLFW_KEY_G,
        h = GLFW_KEY_H,
        i = GLFW_KEY_I,
        j = GLFW_KEY_J,
        k = GLFW_KEY_K,
        l = GLFW_KEY_L,
        m = GLFW_KEY_M,
        n = GLFW_KEY_N,
        o = GLFW_KEY_O,
        p = GLFW_KEY_P,
        q = GLFW_KEY_Q,
        r = GLFW_KEY_R,
        s = GLFW_KEY_S,
        t = GLFW_KEY_T,
        u = GLFW_KEY_U,
        v = GLFW_KEY_V,
        w = GLFW_KEY_W,
        x = GLFW_KEY_X,
        y = GLFW_KEY_Y,
        z = GLFW_KEY_Z,

        _0 = GLFW_KEY_0,
        _1 = GLFW_KEY_1,
        _2 = GLFW_KEY_2,
        _3 = GLFW_KEY_3,
        _4 = GLFW_KEY_4,
        _5 = GLFW_KEY_5,
        _6 = GLFW_KEY_6,
        _7 = GLFW_KEY_7,
        _8 = GLFW_KEY_8,
        _9 = GLFW_KEY_9,

        f1 = GLFW_KEY_F1,
        f2 = GLFW_KEY_F2,
        f3 = GLFW_KEY_F3,
        f4 = GLFW_KEY_F4,
        f5 = GLFW_KEY_F5,
        f6 = GLFW_KEY_F6,
        f7 = GLFW_KEY_F7,
        f8 = GLFW_KEY_F8,
        f9 = GLFW_KEY_F9,
        f10 = GLFW_KEY_F10,
        f11 = GLFW_KEY_F11,
        f12 = GLFW_KEY_F12,

        numpad_0 = GLFW_KEY_KP_0,
        numpad_1 = GLFW_KEY_KP_1,
        numpad_2 = GLFW_KEY_KP_2,
        numpad_3 = GLFW_KEY_KP_3,
        numpad_4 = GLFW_KEY_KP_4,
        numpad_5 = GLFW_KEY_KP_5,
        numpad_6 = GLFW_KEY_KP_6,
        numpad_7 = GLFW_KEY_KP_7,
        numpad_8 = GLFW_KEY_KP_8,
        numpad_9 = GLFW_KEY_KP_9,

        num_lock        = GLFW_KEY_NUM_LOCK,
        numpad_divide   = GLFW_KEY_KP_DIVIDE,
        numpad_multiply = GLFW_KEY_KP_MULTIPLY,
        numpad_subtract = GLFW_KEY_KP_SUBTRACT,
        numpad_add      = GLFW_KEY_KP_ADD,
        numpad_enter    = GLFW_KEY_KP_ENTER,

        left_shift      = GLFW_KEY_LEFT_SHIFT,
        right_shift     = GLFW_KEY_RIGHT_SHIFT,
        left_alt        = GLFW_KEY_LEFT_ALT,
        right_alt       = GLFW_KEY_RIGHT_ALT,
        left_ctrl       = GLFW_KEY_LEFT_CONTROL,
        right_ctrl      = GLFW_KEY_RIGHT_CONTROL,
        left_super      = GLFW_KEY_LEFT_SUPER,
        right_super     = GLFW_KEY_RIGHT_SUPER,
        menu            = GLFW_KEY_MENU,

        space           = GLFW_KEY_SPACE,
        capslock        = GLFW_KEY_CAPS_LOCK,
        escape          = GLFW_KEY_ESCAPE,
        enter           = GLFW_KEY_ENTER,
        tab             = GLFW_KEY_TAB,
        backspace       = GLFW_KEY_BACKSPACE,

        grave_accent    = GLFW_KEY_GRAVE_ACCENT,      // ^ on german
        minus           = GLFW_KEY_MINUS,             // ß on german
        equal           = GLFW_KEY_EQUAL,             // ´ on german
        backslash       = GLFW_KEY_BACKSLASH,         // non-existent key on german?
        left_bracket    = GLFW_KEY_LEFT_BRACKET,      // ü on german
        right_bracket   = GLFW_KEY_RIGHT_BRACKET,     // + on german
        semicolon       = GLFW_KEY_SEMICOLON,         // ö on german
        apostrophe      = GLFW_KEY_APOSTROPHE,        // ä on german
        comma           = GLFW_KEY_COMMA,             // same on german
        period          = GLFW_KEY_PERIOD,            // same on german
        slash           = GLFW_KEY_SLASH,             // - on german

        arrow_up        = GLFW_KEY_UP,
        arrow_left      = GLFW_KEY_LEFT,
        arrow_right     = GLFW_KEY_RIGHT,
        arrow_down      = GLFW_KEY_DOWN,

        ins             = GLFW_KEY_INSERT,
        del             = GLFW_KEY_DELETE,
        home            = GLFW_KEY_HOME,              // pos1 on german
        end             = GLFW_KEY_END,
        page_up         = GLFW_KEY_PAGE_UP,
        page_down       = GLFW_KEY_PAGE_DOWN,

        print           = GLFW_KEY_PRINT_SCREEN,      // druck on german
        scroll_lock     = GLFW_KEY_SCROLL_LOCK,       // rollen on german
        pause           = GLFW_KEY_PAUSE,

        unknown         = GLFW_KEY_UNKNOWN,

        MAX_ENUM = GLFW_KEY_LAST
    };

    /**
     * These have the same numeric value as the corresponding GLFW defines.
     *
     * This is not an enum class because mods have to be combinable
     */
    enum KeyModFlagBits {
        shift     = GLFW_MOD_SHIFT,
        control   = GLFW_MOD_CONTROL,
        alt       = GLFW_MOD_ALT,
        super     = GLFW_MOD_SUPER,
        caps_lock = GLFW_MOD_CAPS_LOCK,
        num_lock  = GLFW_MOD_NUM_LOCK,

        MAX_ENUM
    };

    using KeyModFlags = vk::Flags<KeyModFlagBits>;

    /**
     * These have the same numeric value as the corresponding GLFW defines.
     */
    enum class MouseButton {
        left     = GLFW_MOUSE_BUTTON_LEFT,
        right    = GLFW_MOUSE_BUTTON_RIGHT,
        wheel    = GLFW_MOUSE_BUTTON_MIDDLE,
        middle   = wheel,

        button_1 = GLFW_MOUSE_BUTTON_1,     // same as left
        button_2 = GLFW_MOUSE_BUTTON_2,     // same as right
        button_3 = GLFW_MOUSE_BUTTON_3,     // same as middle
        button_4 = GLFW_MOUSE_BUTTON_4,
        button_5 = GLFW_MOUSE_BUTTON_5,
        button_6 = GLFW_MOUSE_BUTTON_6,
        button_7 = GLFW_MOUSE_BUTTON_7,
        button_8 = GLFW_MOUSE_BUTTON_8,

        MAX_ENUM = GLFW_MOUSE_BUTTON_LAST
    };

    /**
     * These have the same numeric value as the corresponding GLFW defines.
     */
    enum class InputAction {
        release = GLFW_RELEASE,
        press = GLFW_PRESS,
        repeat = GLFW_REPEAT,

        MAX_ENUM
    };
} // namespace vkb



namespace vk
{
    template <>
    struct FlagTraits<::vkb::KeyModFlagBits>
    {
        enum : VkFlags
        {
            allFlags = VkFlags(::vkb::KeyModFlagBits::shift)
                       | VkFlags(::vkb::KeyModFlagBits::control)
                       | VkFlags(::vkb::KeyModFlagBits::alt)
                       | VkFlags(::vkb::KeyModFlagBits::super)
                       | VkFlags(::vkb::KeyModFlagBits::caps_lock)
                       | VkFlags(::vkb::KeyModFlagBits::num_lock)
        };
    };
} // namespace vk

namespace vkb
{
    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        KeyModFlags operator|( KeyModFlagBits bit0,
                               KeyModFlagBits bit1 ) VULKAN_HPP_NOEXCEPT
    {
        return KeyModFlags( bit0 ) | bit1;
    }

    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        KeyModFlags operator&( KeyModFlagBits bit0,
                               KeyModFlagBits bit1)VULKAN_HPP_NOEXCEPT
    {
        return KeyModFlags( bit0 ) & bit1;
    }

    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        KeyModFlags operator^( KeyModFlagBits bit0,
                               KeyModFlagBits bit1 ) VULKAN_HPP_NOEXCEPT
    {
        return KeyModFlags( bit0 ) ^ bit1;
    }

    VULKAN_HPP_INLINE VULKAN_HPP_CONSTEXPR
        KeyModFlags operator~( KeyModFlagBits bits ) VULKAN_HPP_NOEXCEPT
    {
        return ~( KeyModFlags( bits ) );
    }
} // namespace trc
