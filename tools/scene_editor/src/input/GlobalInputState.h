#pragma once

#include "input/KeyboardState.h"
#include "input/MouseState.h"

struct GlobalInputState
{
    KeyboardState keyboard;
    MouseState mouse;
};
