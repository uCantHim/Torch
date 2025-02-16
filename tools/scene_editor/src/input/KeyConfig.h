#pragma once

#include "input/InputHandler.h"
#include "input/UserInput.h"

class Scene;

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
 * @brief Set up key bindings for the main scene viewport.
 */
void setupMainSceneInputFrame(InputFrame& frame, const KeyConfig& conf, s_ptr<Scene> scene);
