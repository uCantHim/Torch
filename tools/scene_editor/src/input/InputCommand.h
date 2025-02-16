#pragma once

#include <functional>

#include <trc/base/event/Keys.h>
#include <trc/Types.h>
using namespace trc::basic_types;

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
