#include "input/InputHandler.h"

#include <cassert>

#include <trc_util/Assert.h>
#include <trc_util/TypeUtils.h>



bool InputFrame::notify(CommandExecutionContext& ctx, const UserInput& input)
{
    // Find a command for the input and execute it.
    if (auto cmd = keyMap.get(input))
    {
        cmd->execute(ctx);
        return true;
    }

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

void CommandExecutionContext::discardEvent()
{
    result.discardEvent = true;
}

auto CommandExecutionContext::keyboard() const -> const KeyboardState&
{
    return *keyboardState;
}

auto CommandExecutionContext::mouse() const -> const MouseState&
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



InputHandler::InputHandler(u_ptr<InputFrame> rootFrame, s_ptr<ActionExecutor> exec)
{
    assert_arg(rootFrame != nullptr);
    assert_arg(exec != nullptr);

    push(std::move(rootFrame));
    actionExecutor = std::move(exec);
}

auto InputHandler::notify(const UserInput& input) -> NotifyResult
{
    // Set the corresponding persistent state.
    std::visit(trc::util::VariantVisitor{
        [this](const KeyInput& input){ keyboard.notify(input.key, input.action); },
        [this](const MouseInput& input){ mouse.notify(input.button, input.action); },
    }, input.input);

    // Execute the respective command.
    CommandExecutionContext ctx{ input, &keyboard, &mouse };
    return processEvent(input, ctx);
}

auto InputHandler::notify(const Scroll& scroll) -> NotifyResult
{
    CommandExecutionContext ctx{ trc::Key::unknown, &keyboard, &mouse };
    return processEvent(scroll, ctx);
}

auto InputHandler::notify(const CursorMovement& cursorMove) -> NotifyResult
{
    // Set the corresponding persistent state.
    mouse.notifyCursorMove(cursorMove.position);

    // Execute the respective command.
    CommandExecutionContext ctx{ trc::Key::unknown, &keyboard, &mouse };
    return processEvent(cursorMove, ctx);
}

auto InputHandler::processEvent(auto&& event, CommandExecutionContext& ctx) -> NotifyResult
{
    if (!top().notify(ctx, event)) {
        return NotifyResult::eRejected;
    }

    auto res = ctx.createResult();
    if (res.discardEvent) {
        return NotifyResult::eRejected;
    }

    processCommandResult(std::move(res));
    return NotifyResult::eConsumed;
}

void InputHandler::processCommandResult(CommandExecutionContext::CommandResult res)
{
    if (top().shouldExit() && frameStack.size() > 1) {
        pop();
    }

    if (res.pushedFrame) {
        push(std::move(res.pushedFrame));
    }

    for (auto& action : res.actions) {
        actionExecutor->execute(std::move(action));
    }
}

auto InputHandler::getRootFrame() -> InputFrame&
{
    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the base frame.");
    assert(frameStack.front() != nullptr && "Root frame must always be valid.");
    return *frameStack.front();
}

void InputHandler::setActionExecutor(s_ptr<ActionExecutor> exec)
{
    assert_arg(exec != nullptr);
    actionExecutor = std::move(exec);
}

void InputHandler::push(u_ptr<InputFrame> frame)
{
    assert(frame != nullptr && "You shall not push nullptrs onto the frame stack.");
    frameStack.emplace_back(std::move(frame));
}

void InputHandler::pop()
{
    frameStack.pop_back();

    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the root frame.");
}

auto InputHandler::top() -> InputFrame&
{
    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the root frame.");
    assert(frameStack.back() != nullptr);
    return *frameStack.back();
}
