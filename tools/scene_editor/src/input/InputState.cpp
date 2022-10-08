#include "InputState.h"



CommandCall::CommandCall(VariantInput input, InputState& state)
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

void CommandCall::on(KeyInput input, u_ptr<InputCommand> cmd)
{
    state->keyMap.set(input, std::move(cmd));
}

void CommandCall::on(MouseInput input, u_ptr<InputCommand> cmd)
{
    state->keyMap.set(input, std::move(cmd));
}

auto CommandCall::setState(u_ptr<CommandState> newState) -> CommandState&
{
    auto& ref = *newState;
    state->setCommandState(std::move(newState));

    return ref;
}

auto CommandCall::getProvokingInput() const -> const VariantInput&
{
    return input;
}



InputState::InputState(VariantInput input, InputCommand& command)
    :
    call(input, *this)
{
    command.execute(call);
}

auto InputState::update(const float timeDelta) -> Status
{
    assert(state != nullptr);

    if (state->update(timeDelta)) {
        return Status::eDone;
    }
    return Status::eInProgress;
}

void InputState::exit()
{
    assert(state != nullptr);
    state->onExit();
}

void InputState::setCommandState(u_ptr<CommandState> newState)
{
    assert(newState != nullptr);
    state = std::move(newState);
}

auto InputState::getKeyMap() -> KeyMap&
{
    return keyMap;
}

void InputState::setKeyMap(KeyMap newMap)
{
    keyMap = std::move(newMap);
}



InputStateMachine::InputStateMachine()
{
    struct EndlessCommandState : CommandState
    {
        bool update(float) override { return false; }
        void onExit() override {}
    };

    struct TopLevelCommand : InputCommand
    {
        void execute(CommandCall& call) override
        {
            call.setState(std::make_unique<EndlessCommandState>());
        }
    };

    TopLevelCommand command;
    executeCommand({ trc::Key::unknown }, command);
}

void InputStateMachine::update(const float timeDelta)
{
    auto status = top().update(timeDelta);
    if (status == InputState::Status::eDone) {
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
    push(std::make_unique<InputState>(input, cmd));
}

void InputStateMachine::push(u_ptr<InputState> state)
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

auto InputStateMachine::top() -> InputState&
{
    assert(!stateStack.empty() && "State stack can never be empty: The first element is the base state.");
    return *stateStack.back();
}
