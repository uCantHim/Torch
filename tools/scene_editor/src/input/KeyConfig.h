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
 * @brief Set up key bindings for the root frame.
 */
void setupRootInputFrame(InputFrame& frame, const KeyConfig& conf, App& app);

/**
 * @brief Set up key bindings for the main scene viewport.
 */
void setupMainSceneInputFrame(InputFrame& frame, const KeyConfig& conf, App& app);
