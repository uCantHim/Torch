#pragma once

#include <memory>
#include <stack>

#include <trc/base/event/Keys.h>

#include "KeyMap.h"

class InputState;

class CommandState
{
public:
    virtual ~CommandState() = default;

    virtual bool update(float timeDelta) = 0;
    virtual void onExit() = 0;
};

/**
 * @brief Interface for building and manipulating command states
 */
class CommandCall
{
private:
    friend class InputState;
    CommandCall(UserInput input, InputState& state);
    ~CommandCall() = default;

public:
    CommandCall(const CommandCall&) = delete;
    CommandCall(CommandCall&&) noexcept = delete;
    auto operator=(const CommandCall&) -> CommandCall& = delete;
    auto operator=(CommandCall&&) noexcept -> CommandCall& = delete;

    void onFirstRepeat(std::function<void()> func);
    void onRepeat(std::function<void()> func);
    void onRelease(std::function<void()> func);

    void on(UserInput input, u_ptr<InputCommand> cmd);

    template<std::invocable<CommandCall&> T>
    void on(UserInput input, T&& func);

    template<std::derived_from<CommandState> T>
    auto setState(T&& t) -> T&;
    auto setState(u_ptr<CommandState> state) -> CommandState&;

    auto getProvokingInput() const -> const UserInput&;

private:
    template<typename T>
    struct FunctionalInputCommand : InputCommand
    {
        FunctionalInputCommand(T&& t) : func(std::forward<T>(t)) {}
        T func;

        void execute(CommandCall& call) override
        {
            func(call);
        };
    };

    UserInput input;
    InputState* state;
};

/**
 * @brief State that determines response to inputs
 *
 * Contains key mappings and per-frame update behaviour.
 *
 * Only used internally in InputStateMachine.
 */
class InputState
{
    /** CommandCall enables manipulation of the CommandState */
    friend class CommandCall;

public:
    enum class Status
    {
        eInProgress,
        eDone,
    };

    /**
     * @brief Create state for a command and execute it
     */
    InputState(UserInput input, InputCommand& command);

    auto update(float timeDelta) -> Status;
    void exit();

    void setCommandState(u_ptr<CommandState> state);
    auto getKeyMap() -> KeyMap&;
    void setKeyMap(KeyMap newMap);

private:
    struct NullCommandState : CommandState
    {
        bool update(float) override { return true; }
        void onExit() override {}
    };

    CommandCall call;
    u_ptr<CommandState> state{ new NullCommandState };

    KeyMap keyMap;
};

/**
 * @brief Stack-based state machine for global input management
 */
class InputStateMachine
{
public:
    InputStateMachine();

    void update(float timeDelta);

    void notify(KeyInput input);
    void notify(MouseInput input);

    auto getKeyMap() -> KeyMap&;
    void setKeyMap(KeyMap map);

private:
    void executeCommand(UserInput input, InputCommand& cmd);

    void push(u_ptr<InputState> state);
    void pop();
    auto top() -> InputState&;

    std::vector<u_ptr<InputState>> stateStack;
};



template<std::invocable<CommandCall&> T>
void CommandCall::on(UserInput input, T&& func)
{
    on(input, std::make_unique<FunctionalInputCommand<T>>(std::forward<T>(func)));
}

template<std::derived_from<CommandState> T>
inline auto CommandCall::setState(T&& t) -> T&
{
    auto ptr = std::make_unique<T>(std::forward<T>(t));
    auto& ref = *ptr;
    state->setCommandState(std::move(ptr));

    return ref;
}
