#pragma once

#include "KeyMap.h"

class App;

struct KeyConfig
{
    UserInput closeApp;
    UserInput openContext;
    UserInput selectHoveredObject;
    UserInput deleteHoveredObject;

    UserInput cameraMove;
    UserInput cameraRotate;

    UserInput translateObject;
    UserInput scaleObject;
    UserInput rotateObject;
};

auto makeKeyMap(App& app, const KeyConfig& conf) -> KeyMap;
