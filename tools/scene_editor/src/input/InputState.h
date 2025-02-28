#pragma once

#include <concepts>
#include <functional>

#include <trc/base/event/Keys.h>
#include <trc_util/Assert.h>

#include "input/Command.h"
#include "input/EventTarget.h"
#include "input/InputFrameBuilder.h"
#include "input/KeyMap.h"
#include "input/KeyboardState.h"
#include "input/MouseState.h"

/**
 * @brief A key mapping state.
 *
 * Maps input events to callbacks.
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
     * @brief Inform that an input event has occurred, and try to execute the
     *        respective command.
     *
     * @return True if the event has been handled, false otherwise.
     */
    bool notify(CommandExecutionContext& ctx, const UserInput& input);

    /**
     * @brief Inform that a scroll event has occurred, and try to execute the
     *        respective command.
     *
     * @return True if the event has been handled, false otherwise.
     */
    bool notify(CommandExecutionContext& ctx, const Scroll& scroll);

    /**
     * @brief Inform that a cursor move event has occurred, and try to execute
     *        the respective command.
     *
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

    CommandExecutionContext(const UserInput& provokingInput,
                            const KeyboardState* curKeyboard,
                            const MouseState* curMouse);

    /**
     * @return The input that caused the command to be executed. Is the same as
     *         `KeyInput{ trc::Key::unknown }` if the command was not executed
     *         as a result of an input event (this is the case for mouse
     *         movement events, for example).
     */
    auto getProvokingInput() const -> UserInput;

    /**
     * @return The keyboard state of the current viewport.
     */
    auto keyboard() -> const KeyboardState&;

    /**
     * @return The mouse state of the current viewport.
     */
    auto mouse() -> const MouseState&;

    /**
     * @brief Schedule an invertible action for execution.
     *
     * On command completion, all generated actions will be pushed onto the
     * action history and executed in order.
     */
    void generateAction(u_ptr<InvertibleAction> action);

    /**
     * @brief Push an empty input frame onto the input stack.
     *
     * The new frame will supersede the currently active frame and handle all
     * future events. When it exits, it will be removed from the frame stack and
     * return control to the previously active frame.
     *
     * @throw std::out_of_range if another frame has already been pushed onto
     *                          the command context.
     */
    auto pushFrame() -> InputFrameBuilder<InputFrame> {
        return pushFrame(std::make_unique<InputFrame>());
    }

    /**
     * @brief Push an input frame onto the input stack.
     *
     * This does the same thing as the `pushFrame(u_ptr<Frame>)` overload, but
     * enables better intellisense and type deduction at the call site for both
     * the derived frame type's constructor and the returned frame builder.
     *
     * @throw std::invalid_argument if `frame == nullptr`.
     * @throw std::out_of_range if another frame has already been pushed onto
     *                          the command context.
     */
    template<std::derived_from<InputFrame> Frame>
        requires std::constructible_from<std::remove_cvref_t<Frame>, Frame>
    auto pushFrame(Frame&& frame) -> InputFrameBuilder<Frame>
    {
        using FrameT = std::remove_cvref_t<Frame>;
        checkNoPushedFrame();

        result.pushedFrame = std::make_unique<FrameT>(std::forward<Frame>(frame));
        return InputFrameBuilder{ static_cast<FrameT*>(result.pushedFrame.get()) };
    }

    /**
     * @brief Push an input frame onto the input stack.
     *
     * The new frame will supersede the currently active frame and handle all
     * future events. When it exits, it will be removed from the frame stack and
     * return control to the previously active frame.
     *
     * One may pass an object of a derived class `T : InputFrame` as the new
     * frame. In that case, `T` can be used to store arbitrary state data that
     * one wants to access across event callbacks.
     *
     * Example:
     * @code
     *   struct MyState : InputFrame {
     *       std::string msg;
     *   };
     *
     *   // Create a command that pushes a new input frame
     *   rootInput.on(trc::Key::i, [](CommandExecutionContext& ctx) {
     *       // Push our own state object as an input frame
     *       auto state = ctx.pushState(std::make_unique<MyState>());
     *
     *       // Create some commands on the new input frame
     *       state.on(trc::Key::a, [](MyState& state, CommandExecutionContext&) {
     *           state.msg += 'a';
     *       });
     *       state.on(trc::Key::escape, [](MyState& state, CommandExecutionContext&) {
     *           std::cout << "A new message was built: " << state.msg << "\n";
     *           state.exitFrame();
     *       });
     *   });
     * @endcode
     *
     * @param frame The new input frame. Must not be `nullptr`.
     *
     * @return A typed `InputFrame` interface that allows access to custom state
     *         types in input callbacks.
     * @throw std::invalid_argument if `frame == nullptr`.
     * @throw std::out_of_range if another frame has already been pushed onto
     *                          the command context.
     */
    template<std::derived_from<InputFrame> Frame>
    auto pushFrame(u_ptr<Frame> frame) -> InputFrameBuilder<Frame>
    {
        assert_arg(frame != nullptr);
        checkNoPushedFrame();

        result.pushedFrame = std::move(frame);
        return InputFrameBuilder{ static_cast<Frame*>(result.pushedFrame.get()) };
    }

private:
    friend class InputStateMachine;

    /**
     * @brief The result of a command execution.
     *
     * Communicates results to InputStateMachine.
     */
    struct CommandResult
    {
        // A frame designated to be pushed onto the input frame stack.
        u_ptr<InputFrame> pushedFrame;

        // A list of actions generated by the command. These shall be executed
        // and pushed to the command history.
        std::vector<u_ptr<InvertibleAction>> actions;
    };

    auto createResult() -> CommandResult;

    /** @throw std::out_of_range */
    void checkNoPushedFrame();

private:
    UserInput provokingInput;
    const KeyboardState* keyboardState;
    const MouseState* mouseState;

    CommandResult result;
};

/**
 * @brief A stack of input frames that handles input events.
 */
class InputStateMachine : public EventTarget
{
public:
    InputStateMachine();

    /**
     * @brief Initialize with a preconstructed root frame.
     *
     * @param topLevelFrame Must not be `nullptr`.
     *
     * @throw std::invalid_argument if `topLevelFrame == nullptr`.
     */
    explicit InputStateMachine(u_ptr<InputFrame> topLevelFrame);

    /**
     * @brief Signal the occurrence of an input event.
     */
    auto notify(const UserInput& input) -> NotifyResult override;

    /**
     * @brief Signal the occurrence of a scroll event.
     */
    auto notify(const Scroll& scroll) -> NotifyResult override;

    /**
     * @brief Signal the occurrence of a cursor move event.
     */
    auto notify(const CursorMovement& cursorMove) -> NotifyResult override;

    /**
     * @brief Get the root frame of this state stack.
     *
     * The root frame will remain valid forever as it cannot be exited. Query
     * the root frame to configure input callbacks on it.
     */
    auto getRootFrame() -> InputFrame&;

private:
    void processCommandResult(CommandExecutionContext::CommandResult result);

    void push(u_ptr<InputFrame> frame);
    void pop();
    auto top() -> InputFrame&;

    std::vector<u_ptr<InputFrame>> frameStack;
    KeyboardState keyboard;
    MouseState mouse;
};
