#pragma once

#include <vkb/event/Keys.h>
#include <trc/Types.h>
using namespace trc::basic_types;

struct KeyInput
{
    KeyInput(vkb::Key key);
    KeyInput(vkb::Key key, vkb::KeyModFlags mods);

    vkb::Key key;
    vkb::KeyModFlags mod;
};

struct MouseInput
{
    MouseInput(vkb::MouseButton button);
    MouseInput(vkb::MouseButton button, vkb::KeyModFlags mods);

    vkb::MouseButton button;
    vkb::KeyModFlags mod;
};



inline bool operator==(const KeyInput& a, const KeyInput& b)
{
    return a.key == b.key && a.mod == b.mod;
}

inline bool operator==(const MouseInput& a, const MouseInput& b)
{
    return a.button == b.button && a.mod == b.mod;
}

template<>
struct std::hash<KeyInput>
{
    inline auto operator()(const KeyInput& val) const noexcept
    {
        const ui32 h = (static_cast<ui32>(val.key) << 16) | (static_cast<ui8>(val.mod));
        return hash<ui32>{}(h);
    }
};

template<>
struct std::hash<MouseInput>
{
    inline auto operator()(const MouseInput& val) const noexcept
    {
        const ui32 h = (static_cast<ui32>(val.button) << 8) | (static_cast<ui8>(val.mod));
        return hash<ui32>{}(h);
    }
};
