#pragma once

#include <memory>
#include <functional>
#include <variant>

#include <vkb/event/Keys.h>
#include <trc/Types.h>
using namespace trc::basic_types;

#include "InputStructs.h"

class App;
class CommandCall;

class InputCommand
{
public:
    virtual void execute(CommandCall& call) = 0;
    virtual ~InputCommand() = default;
};

auto makeInputCommand(std::function<void()> execute) -> u_ptr<InputCommand>;
auto makeInputCommand(std::function<void(CommandCall&)> execute) -> u_ptr<InputCommand>;
