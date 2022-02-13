#pragma once

#include "KeyMap.h"

class App;

struct KeyConfig
{
    VariantInput closeApp;
    VariantInput openContext;
    VariantInput selectHoveredObject;

    VariantInput translateObject;
    VariantInput scaleObject;
    VariantInput rotateObject;
};

auto makeKeyMap(App& app, const KeyConfig& conf) -> KeyMap;
