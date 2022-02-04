#pragma once

#include "KeyMap.h"

class App;

struct KeyConfig
{
    KeyInput closeApp;
    MouseInput openContext;
};

auto makeKeyMap(App& app, const KeyConfig& conf) -> KeyMap;
