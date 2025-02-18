#include "InputState.h"

#include <iostream>

#include <trc_util/Assert.h>
#include <trc_util/TypeUtils.h>



auto InputFrame::notify(const UserInput& input) -> std::variant<action::None, action::PushFrame>
{
    auto tryExecCallback = [](auto&& input, auto&& key, auto&& mouse) {
        using Res = std::variant<action::None, action::PushFrame>;

        CommandExecutor exec{ input };
        return std::visit(trc::util::VariantVisitor{
            [&](const KeyInput& input) -> Res {
                if (key) return exec.executeCommand([&](auto& ctx){ key(ctx, input); });
                return action::None{};
            },
            [&](const MouseInput& input) -> Res {
                if (mouse) return exec.executeCommand([&](auto& ctx){ mouse(ctx, input); });
                return action::None{};
            },
        }, input.input);
    };

    std::cout << "Input frame: processing input event...\n";

    // Find a command for the input and execute it.
    CommandExecutor exec{ input };
    if (auto cmd = keyMap.get(input)) {
        std::cout << "Command is set for this event. Executing it...\n";
        return exec.executeCommand(*cmd);
    }

    // No command is set for the input. Forward it to the catch-all handler.
    std::cout << "No command registered. Calling catch-all callback...\n";
    return tryExecCallback(input, unhandledKeyCallback, unhandledMouseCallback);
}

auto InputFrame::notify(const Scroll& scroll) -> std::variant<action::None, action::PushFrame>
{
    const KeyInput noKey{ trc::Key::unknown };

    if (scrollCallback)
    {
        auto cmd = [this, scroll](CommandExecutionContext& ctx) {
            this->scrollCallback(ctx, scroll);
        };
        return CommandExecutor{ noKey }.executeCommand(cmd);
    }
    return action::None{};
}

auto InputFrame::notify(const CursorMovement& cursorMove)
    -> std::variant<action::None, action::PushFrame>
{
    const KeyInput noKey{ trc::Key::unknown };

    if (cursorMoveCallback)
    {
        auto cmd = [this, cursorMove](CommandExecutionContext& ctx) {
            this->cursorMoveCallback(ctx, cursorMove);
        };
        return CommandExecutor{ noKey }.executeCommand(cmd);
    }
    return action::None{};
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



CommandExecutor::CommandExecutor(const UserInput& provokingInput)
    : provokingInput(provokingInput)
{}

auto CommandExecutor::executeCommand(Command& cmd)
    -> std::variant<action::None, action::PushFrame>
{
    return executeCommand([&cmd](auto& ctx){ cmd.execute(ctx); });
}



CommandExecutionContext::CommandExecutionContext(const UserInput& provokingInput)
    :
    provokingInput(provokingInput)
{}

auto CommandExecutionContext::getProvokingInput() -> UserInput
{
    return provokingInput;
}

auto CommandExecutionContext::pushFrame() -> GenericInputFrameBuilder
{
    return { pushFrame(std::make_unique<GenericInputFrame>()) };
}

bool CommandExecutionContext::hasFramePushed() const
{
    return nextFrame != nullptr;
}

auto CommandExecutionContext::getFrame() && -> u_ptr<InputFrame>
{
    return std::move(nextFrame);
}



InputStateMachine::InputStateMachine()
    : InputStateMachine(std::make_unique<CommandExecutionContext::GenericInputFrame>())
{
}

InputStateMachine::InputStateMachine(u_ptr<InputFrame> topLevelFrame)
{
    assert_arg(topLevelFrame != nullptr);

    push(std::move(topLevelFrame));
}

void InputStateMachine::update(const float timeDelta)
{
    auto& frame = top();

    frame.onTick(timeDelta);
    if (frame.shouldExit()) {
        pop();
    }
}

void InputStateMachine::notify(const UserInput& input)
{
    auto result = top().notify(input);

    if (top().shouldExit()) {
        pop();
    }
    std::visit(trc::util::VariantVisitor{
        [](action::None){},
        [this](action::PushFrame& frame){ push(std::move(frame.newFrame)); },
    }, result);
}

void InputStateMachine::notify(const Scroll& scroll)
{
    auto result = top().notify(scroll);

    if (top().shouldExit()) {
        pop();
    }
    std::visit(trc::util::VariantVisitor{
        [](action::None){},
        [this](action::PushFrame& frame){ push(std::move(frame.newFrame)); },
    }, result);
}

void InputStateMachine::notify(const CursorMovement& cursorMove)
{
    auto result = top().notify(cursorMove);

    if (top().shouldExit()) {
        pop();
    }
    std::visit(trc::util::VariantVisitor{
        [](action::None){},
        [this](action::PushFrame& frame){ push(std::move(frame.newFrame)); },
    }, result);
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
    top().onExit();
    frameStack.pop_back();

    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the root frame.");
}

auto InputStateMachine::top() -> InputFrame&
{
    assert(!frameStack.empty() && "Frame stack can never be empty: The first element is the root frame.");
    assert(frameStack.back() != nullptr);
    return *frameStack.back();
}
