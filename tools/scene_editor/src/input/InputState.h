#pragma once

#include <concepts>
#include <functional>

#include <trc/base/event/Keys.h>
#include <trc_util/Assert.h>

#include "input/Command.h"
#include "input/EventTarget.h"
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

/**
 * Maps keys to callbacks.
 */
class InputFrame
{
public:
    InputFrame(const InputFrame&) = delete;
    InputFrame& operator=(const InputFrame&) = delete;
    InputFrame& operator=(InputFrame&&) noexcept = delete;

    InputFrame() = default;
    InputFrame(InputFrame&&) noexcept = default;
    ~InputFrame() noexcept = default;

    /**
     * @return True if the event has been handled, false otherwise.
     */
    bool notify(CommandExecutionContext& ctx, const UserInput& input);

    /**
     * @return True if the event has been handled, false otherwise.
     */
    bool notify(CommandExecutionContext& ctx, const Scroll& scroll);

    /**
     * @return True if the event has been handled, false otherwise.
     */
    bool notify(CommandExecutionContext& ctx, const CursorMovement& cursorMove);

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

struct CommandResult
{
    u_ptr<InputFrame> pushedFrame;
    std::vector<u_ptr<InvertibleAction>> actions;
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

    auto getProvokingInput() const -> UserInput;

    // auto keyboard();
    // auto mouse();

    void generateAction(u_ptr<InvertibleAction> action);

    auto pushFrame() -> InputFrameBuilder<InputFrame> {
        return pushFrame(std::make_unique<InputFrame>());
    }

    /**
     * Mostly this overload enables better intellisense and type deduction at
     * the call size, for both the derived frame type's constructor and the
     * returned frame builder.
     */
    template<std::derived_from<InputFrame> Frame>
        requires std::constructible_from<std::remove_cvref_t<Frame>, Frame>
    auto pushFrame(Frame&& frame) -> InputFrameBuilder<Frame>
    {
        using FrameT = std::remove_cvref_t<Frame>;

        nextFrame = std::make_unique<FrameT>(std::forward<Frame>(frame));
        return InputFrameBuilder{ static_cast<FrameT*>(nextFrame.get()) };
    }

    /**
     * @throw std::invalid_argument if `frame == nullptr`.
     */
    template<std::derived_from<InputFrame> Frame>
    auto pushFrame(u_ptr<Frame> frame) -> InputFrameBuilder<Frame>
    {
        assert_arg(frame != nullptr);

        nextFrame = std::move(frame);
        return InputFrameBuilder{ static_cast<Frame*>(nextFrame.get()) };
    }

    bool hasFramePushed() const;

private:
    friend class InputStateMachine;
    auto createResult() -> CommandResult;

private:
    UserInput provokingInput;
    u_ptr<InputFrame> nextFrame{ nullptr };
    std::vector<u_ptr<InvertibleAction>> actions;
};

/**
 * @brief Stack-based state machine for global input management
 */
class InputStateMachine : public EventTarget
{
public:
    InputStateMachine();
    explicit InputStateMachine(u_ptr<InputFrame> topLevelFrame);

    auto notify(const UserInput& input) -> NotifyResult override;
    auto notify(const Scroll& scroll) -> NotifyResult override;
    auto notify(const CursorMovement& cursorMove) -> NotifyResult override;

    auto getRootFrame() -> InputFrame&;

    static auto build() -> std::pair<InputStateMachine, InputFrame&>;

private:
    void processCommandResult(CommandResult result);

    void push(u_ptr<InputFrame> frame);
    void pop();
    auto top() -> InputFrame&;

    std::vector<u_ptr<InputFrame>> frameStack;
};
