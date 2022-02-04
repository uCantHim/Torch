#include "InputCommand.h"

#include "InputState.h"



struct FunctionalInputCommand : InputCommand
{
    FunctionalInputCommand(std::function<void(CommandCall&)> e)
        : _execute(std::move(e))
    {}

    void execute(CommandCall& call) override
    {
        return _execute(call);
    }

    std::function<void(CommandCall&)> _execute;
};

auto makeInputCommand(std::function<void()> execute) -> u_ptr<InputCommand>
{
    return std::make_unique<FunctionalInputCommand>([=](CommandCall&) -> u_ptr<CommandState> {
        execute();
        return nullptr;
    });
}

auto makeInputCommand(std::function<void(CommandCall&)> execute) -> u_ptr<InputCommand>
{
    return std::make_unique<FunctionalInputCommand>([=](CommandCall& call) -> u_ptr<CommandState> {
        execute(call);
        return nullptr;
    });
}
