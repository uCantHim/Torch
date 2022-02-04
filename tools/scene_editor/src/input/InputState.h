#pragma once

#include <memory>
#include <stack>

#include <vkb/event/Keys.h>

#include "KeyMap.h"

class CommandState;

struct VariantInput
{
    template<typename T>
    static constexpr bool keyOrMouseInput = std::same_as<T, KeyInput> || std::same_as<T, MouseInput>;

    VariantInput(KeyInput in) : input(in) {}
    VariantInput(MouseInput in) : input(in) {}

    template<typename T> requires keyOrMouseInput<T>
    bool is() const
    {
        return std::holds_alternative<T>(input);
    }

    template<typename T> requires keyOrMouseInput<T>
    auto get() const -> T
    {
        if (!is<T>()) throw trc::Exception("Input is not of the requested type");
        return std::get<T>(input);
    }

    std::variant<KeyInput, MouseInput> input;
};

/**
 * @brief Interface for manipulating command states
 */
class CommandCall
{
private:
    friend class CommandState;
    CommandCall(VariantInput input, CommandState& state);
    ~CommandCall() = default;

public:
    CommandCall(const CommandCall&) = delete;
    CommandCall(CommandCall&&) noexcept = delete;
    auto operator=(const CommandCall&) -> CommandCall& = delete;
    auto operator=(CommandCall&&) noexcept -> CommandCall& = delete;

    void onFirstRepeat(std::function<void()> func);
    void onRepeat(std::function<void()> func);
    void onRelease(std::function<void()> func);

    void onUpdate(std::function<bool(float)> func);
    void onExit(std::function<void()> func);

    auto getProvokingInput() const -> const VariantInput&;

private:
    VariantInput input;
    CommandState* state;
};

/**
 * @brief State that determines response to inputs
 *
 * Contains key mappings and per-frame update behaviour.
 */
class CommandState
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
    CommandState(VariantInput input, InputCommand& command);

    auto update(float timeDelta) -> Status;
    void exit();

    auto getKeyMap() -> KeyMap&;
    void setKeyMap(KeyMap newMap);

private:
    CommandCall call;

    std::function<bool(float)> onUpdate;
    std::function<void()> onExit;

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
    void executeCommand(VariantInput input, InputCommand& cmd);

    void push(u_ptr<CommandState> state);
    void pop();
    auto top() -> CommandState&;

    std::vector<u_ptr<CommandState>> stateStack;
};
