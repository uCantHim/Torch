#pragma once

#include <variant>

#include "GraphManipulator.h"
#include "GraphScene.h"

class ControlState;
struct ControlInput;
struct ControlOutput;

struct PushState
{
    u_ptr<ControlState> newState;
};
struct PopState {};
struct NoAction {};

using StateResult = std::variant<NoAction, PushState, PopState>;

class ControlState
{
public:
    virtual ~ControlState() = default;
    virtual auto update(const ControlInput& in, ControlOutput& out)
        -> StateResult = 0;
};
