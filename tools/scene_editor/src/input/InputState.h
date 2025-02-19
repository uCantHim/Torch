#pragma once

#include <concepts>
#include <functional>

#include <trc/base/event/Keys.h>
#include <trc_util/Assert.h>

#include "input/Command.h"
#include "input/InputFrameBuilder.h"
#include "input/KeyMap.h"

class InputFrame;

namespace action
{
    struct PushFrame {
        u_ptr<InputFrame> newFrame;
    };

    struct None {};
};

class InputFrame
{
public:
    InputFrame(const InputFrame&) = delete;
    InputFrame(InputFrame&&) noexcept = delete;
    InputFrame& operator=(const InputFrame&) = delete;
    InputFrame& operator=(InputFrame&&) noexcept = delete;

    InputFrame() = default;
    virtual ~InputFrame() noexcept = default;

    virtual void onExit() = 0;

    auto notify(const UserInput& input) -> std::variant<action::None, action::PushFrame>;
    auto notify(const Scroll& scroll) -> std::variant<action::None, action::PushFrame>;
    auto notify(const CursorMovement& cursorMove) -> std::variant<action::None, action::PushFrame>;

    void on(UserInput input, u_ptr<Command> command);

    template<std::invocable F>
    void on(UserInput input, F&& callback) {
        on(input, makeCommand(std::forward<F>(callback)));
    }

    template<CommandFunctionT F>
    void on(UserInput input, F&& callback) {
        on(input, makeCommand(std::forward<F>(callback)));
    }

    template<std::invocable<CommandExecutionContext&, KeyInput> F>
    void onUnhandledKeyInput(F&& callback) {
        unhandledKeyCallback = std::forward<F>(callback);
    }

    template<std::invocable<CommandExecutionContext&, MouseInput> F>
    void onUnhandledMouseInput(F&& callback) {
        unhandledMouseCallback = std::forward<F>(callback);
    }

    template<std::invocable<CommandExecutionContext&, Scroll> F>
    void onScroll(F&& callback) {
        scrollCallback = std::forward<F>(callback);
    }

    template<std::invocable<CommandExecutionContext&, CursorMovement> F>
    void onCursorMove(F&& callback) {
        cursorMoveCallback = std::forward<F>(callback);
    }

    /**
     * TODO: Idea
     */
    void applyCommand(auto&& _do, auto&& _undo);

    void exitFrame();
    bool shouldExit() const;

private:
    KeyMap keyMap;

    std::function<void(CommandExecutionContext&, KeyInput)> unhandledKeyCallback;
    std::function<void(CommandExecutionContext&, MouseInput)> unhandledMouseCallback;

    std::function<void(CommandExecutionContext&, Scroll)> scrollCallback;
    std::function<void(CommandExecutionContext&, CursorMovement)> cursorMoveCallback;

    bool _shouldExit{ false };
};

/**
 * Isolates command execution logic, which requires dubious access of private
 * members in CommandExecutionContext.
 */
class CommandExecutor
{
public:
    explicit CommandExecutor(const UserInput& provokingInput);

    auto executeCommand(Command& cmd) -> std::variant<action::None, action::PushFrame>;
    auto executeCommand(CommandFunctionT auto&& cmdFunc)
        -> std::variant<action::None, action::PushFrame>;

private:
    UserInput provokingInput;
};

/**
 * @brief Information provided to a command invocation.
 */
class CommandExecutionContext
{
public:
    CommandExecutionContext(const CommandExecutionContext&) = delete;
    CommandExecutionContext(CommandExecutionContext&&) noexcept = delete;
    CommandExecutionContext& operator=(const CommandExecutionContext&) = delete;
    CommandExecutionContext& operator=(CommandExecutionContext&&) noexcept = delete;

    ~CommandExecutionContext() noexcept = default;

    explicit CommandExecutionContext(const UserInput& provokingInput);

    auto getProvokingInput() -> UserInput;

    // auto keyboard();
    // auto mouse();

    class GenericInputFrame : public InputFrame
    {
    public:
        void onExit() final {}

    private:
        std::function<void()> _onExit;
    };

    class GenericInputFrameBuilder : public InputFrameBuilder<GenericInputFrame>
    {
    public:
        void onExit(std::function<void(InputFrame&)> callback);
    };

    auto pushFrame() -> GenericInputFrameBuilder;

    template<std::derived_from<InputFrame> Frame>
    auto pushFrame(u_ptr<Frame> frame) -> InputFrameBuilder<Frame>
    {
        assert_arg(frame != nullptr);

        nextFrame = std::move(frame);
        return InputFrameBuilder{ static_cast<Frame*>(nextFrame.get()) };
    }

    bool hasFramePushed() const;

private:
    friend CommandExecutor;
    auto getFrame() && -> u_ptr<InputFrame>;

private:
    UserInput provokingInput;
    u_ptr<InputFrame> nextFrame{ nullptr };
};

/**
 * @brief Stack-based state machine for global input management
 */
class InputStateMachine
{
public:
    InputStateMachine();
    explicit InputStateMachine(u_ptr<InputFrame> topLevelFrame);

    void notify(const UserInput& input);
    void notify(const Scroll& scroll);
    void notify(const CursorMovement& cursorMove);

    auto getRootFrame() -> InputFrame&;

    static auto build() -> std::pair<InputStateMachine, InputFrame&>;

private:
    void push(u_ptr<InputFrame> frame);
    void pop();
    auto top() -> InputFrame&;

    std::vector<u_ptr<InputFrame>> frameStack;
};



inline auto CommandExecutor::executeCommand(CommandFunctionT auto&& cmdFunc)
    -> std::variant<action::None, action::PushFrame>
{
    CommandExecutionContext ctx{ provokingInput };
    cmdFunc(ctx);

    if (ctx.hasFramePushed()) {
        return action::PushFrame{ .newFrame=std::move(ctx).getFrame() };
    }
    return action::None{};
}
