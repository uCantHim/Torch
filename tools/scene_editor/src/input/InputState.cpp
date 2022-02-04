#include "InputState.h"



CommandCall::CommandCall(VariantInput input, CommandState& state)
    :
    input(input),
    state(&state)
{
}

void CommandCall::onFirstRepeat(std::function<void()>)
{
    throw trc::Exception("Not implemented");
}

void CommandCall::onRepeat(std::function<void()>)
{
    throw trc::Exception("Not implemented");
}

void CommandCall::onRelease(std::function<void()>)
{
    throw trc::Exception("Not implemented");
}

void CommandCall::onUpdate(std::function<bool(float)> func)
{
    state->onUpdate = std::move(func);
}

void CommandCall::onExit(std::function<void()> func)
{
    state->onExit = std::move(func);
}

auto CommandCall::getProvokingInput() const -> const VariantInput&
{
    return input;
}



CommandState::CommandState(VariantInput input, InputCommand& command)
    :
    call(input, *this),
    onUpdate([](float){ return true; }),
    onExit([]{})
{
    command.execute(call);
}

auto CommandState::update(const float timeDelta) -> Status
{
    if (onUpdate(timeDelta)) {
        return Status::eDone;
    }
    return Status::eInProgress;
}

void CommandState::exit()
{
    onExit();
}

auto CommandState::getKeyMap() -> KeyMap&
{
    return keyMap;
}

void CommandState::setKeyMap(KeyMap newMap)
{
    keyMap = std::move(newMap);
}



InputStateMachine::InputStateMachine()
{
    struct TopLevelCommand : InputCommand
    {
        void execute(CommandCall& call) override
        {
            call.onUpdate([](float) { return false; });  // Never finish the command
        }
    };

    TopLevelCommand command;
    executeCommand({ vkb::Key::unknown }, command);
}

void InputStateMachine::update(const float timeDelta)
{
    auto status = top().update(timeDelta);
    if (status == CommandState::Status::eDone) {
        pop();
    }
}

void InputStateMachine::notify(KeyInput input)
{
    auto cmd = top().getKeyMap().get(input);
    if (cmd != nullptr)
    {
        executeCommand(input, *cmd);
    }
}

void InputStateMachine::notify(MouseInput input)
{
    auto cmd = top().getKeyMap().get(input);
    if (cmd != nullptr)
    {
        executeCommand(input, *cmd);
    }
}

auto InputStateMachine::getKeyMap() -> KeyMap&
{
    assert(!stateStack.empty() && "State stack can never be empty: The first element is the base state.");
    return stateStack.front()->getKeyMap();
}

void InputStateMachine::setKeyMap(KeyMap map)
{
    top().setKeyMap(std::move(map));
}

void InputStateMachine::executeCommand(VariantInput input, InputCommand& cmd)
{
    push(std::make_unique<CommandState>(input, cmd));
}

void InputStateMachine::push(u_ptr<CommandState> state)
{
    stateStack.emplace_back(std::move(state));
}

void InputStateMachine::pop()
{
    assert(!stateStack.empty());

    top().exit();
    stateStack.pop_back();

    assert(!stateStack.empty() && "State stack can never be empty: The first element is the base state.");
}

auto InputStateMachine::top() -> CommandState&
{
    assert(!stateStack.empty() && "State stack can never be empty: The first element is the base state.");
    return *stateStack.back();
}
