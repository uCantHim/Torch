#pragma once

#include "input/InputState.h"

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

/**
 * @brief Create a root input frame from a key configuration.
 */
void setupRootInputFrame(InputFrame& frame, const KeyConfig& conf, App& app);
