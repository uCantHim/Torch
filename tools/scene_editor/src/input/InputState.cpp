#include "InputState.h"

#include <cassert>
#include <iostream>

#include <trc_util/Assert.h>
#include <trc_util/TypeUtils.h>



bool InputFrame::notify(CommandExecutionContext& ctx, const UserInput& input)
{
    std::cout << "Input frame: processing input event...\n";

    // Find a command for the input and execute it.
    if (auto cmd = keyMap.get(input))
    {
        std::cout << "Command is set for this event. Executing it...\n";
        cmd->execute(ctx);
        return true;
    }

    std::cout << "No command registered. Calling catch-all callback...\n";

    // No command is set for the input. Forward it to the catch-all handler, if
    // one is registered.
    return std::visit(trc::util::VariantVisitor{
        [&](const KeyInput& input) {
            if (unhandledKeyCallback) unhandledKeyCallback(ctx, input);
            return !!unhandledKeyCallback;
        },
        [&](const MouseInput& input) {
            if (unhandledMouseCallback) unhandledMouseCallback(ctx, input);
            return !!unhandledMouseCallback;
        },
    }, input.input);
}

bool InputFrame::notify(CommandExecutionContext& ctx, const Scroll& scroll)
{
    if (scrollCallback)
    {
        scrollCallback(ctx, scroll);
        return true;
    }
    return false;
}

bool InputFrame::notify(CommandExecutionContext& ctx, const CursorMovement& cursorMove)
{
    if (cursorMoveCallback)
    {
        cursorMoveCallback(ctx, cursorMove);
        return true;
    }
    return false;
}

void InputFrame::on(UserInput input, u_ptr<Command> command)
{
    if (command != nullptr) {
        keyMap.set(input, std::move(command));
    }
    else {
        keyMap.unset(input);
    }
}

void InputFrame::exitFrame()
{
    _shouldExit = true;
}

bool InputFrame::shouldExit() const
{
    return _shouldExit;
}



CommandExecutionContext::CommandExecutionContext(
    const UserInput& provokingInput,
    const KeyboardState* curKeyboard,
    const MouseState* curMouse)
    :
    provokingInput(provokingInput),
    keyboardState(curKeyboard),
    mouseState(curMouse)
{}

auto CommandExecutionContext::getProvokingInput() const -> UserInput
{
    return provokingInput;
}

auto CommandExecutionContext::keyboard() -> const KeyboardState&
{
    return *keyboardState;
}

auto CommandExecutionContext::mouse() -> const MouseState&
{
    return *mouseState;
}

void CommandExecutionContext::generateAction(u_ptr<InvertibleAction> action)
{
    if (action != nullptr) {
        result.actions.emplace_back(std::move(action));
    }
}

auto CommandExecutionContext::createResult() -> CommandResult
{
    return std::move(result);
}

void CommandExecutionContext::checkNoPushedFrame()
{
    if (result.pushedFrame != nullptr) {
        throw std::out_of_range("[In CommandExecutionContext::pushFrame]: Cannot"
                                " push more than one frame onto a command context.");
    }
}



InputStateMachine::InputStateMachine()
    : InputStateMachine(std::make_unique<InputFrame>())
{
}

InputStateMachine::InputStateMachine(u_ptr<InputFrame> topLevelFrame)
{
    assert_arg(topLevelFrame != nullptr);
    push(std::move(topLevelFrame));
}

auto InputStateMachine::notify(const UserInput& input) -> NotifyResult
{
    // Set the corresponding persistent state.
    std::visit(trc::util::VariantVisitor{
        [this](const KeyInput& input){ keyboard.notify(input.key, input.action); },
        [this](const MouseInput& input){ mouse.notify(input.button, input.action); },
    }, input.input);

    // Execute the respective command.
    CommandExecutionContext ctx{ input, &keyboard, &mouse };
    if (!top().notify(ctx, input)) {
        return NotifyResult::eRejected;
    }
    processCommandResult(ctx.createResult());

    return NotifyResult::eConsumed;
}

auto InputStateMachine::notify(const Scroll& scroll) -> NotifyResult
{
    CommandExecutionContext ctx{ trc::Key::unknown, &keyboard, &mouse };
    if (!top().notify(ctx, scroll)) {
        return NotifyResult::eRejected;
    }
    processCommandResult(ctx.createResult());

    return NotifyResult::eConsumed;
}

auto InputStateMachine::notify(const CursorMovement& cursorMove) -> NotifyResult
{
    mouse.notifyCursorMove(cursorMove.position);

    // Execute the respective command.
    CommandExecutionContext ctx{ trc::Key::unknown, &keyboard, &mouse };
    if (!top().notify(ctx, cursorMove)) {
        return NotifyResult::eRejected;
    }
    processCommandResult(ctx.createResult());

    return NotifyResult::eConsumed;
}

void InputStateMachine::processCommandResult(CommandExecutionContext::CommandResult res)
{
    if (top().shouldExit() && frameStack.size() > 1) {
        pop();
    }

    if (res.pushedFrame) {
        push(std::move(res.pushedFrame));
    }

    for (auto& action : res.actions)
    {
        action->apply();
        // Push onto history
    }
}

auto InputStateMachine::getRootFrame() -> InputFrame&
{
    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the base frame.");
    assert(frameStack.front() != nullptr && "Root frame must always be valid.");
    return *frameStack.front();
}

void InputStateMachine::push(u_ptr<InputFrame> frame)
{
    assert(frame != nullptr && "You shall not push nullptrs onto the frame stack.");
    frameStack.emplace_back(std::move(frame));
}

void InputStateMachine::pop()
{
    frameStack.pop_back();

    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the root frame.");
}

auto InputStateMachine::top() -> InputFrame&
{
    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the root frame.");
    assert(frameStack.back() != nullptr);
    return *frameStack.back();
}
