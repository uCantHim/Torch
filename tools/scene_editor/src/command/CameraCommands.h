#pragma once

#include "App.h"
#include "input/Command.h"
#include "input/InputState.h"

class CameraMoveCommand : public Command
{
public:
    struct State : public InputFrame
    {
        static constexpr float kDragSpeed{ 5.0f };
        App* app;
    };

    explicit CameraMoveCommand(App& app) : app(&app) {}

    void execute(CommandExecutionContext& ctx) override
    {
        constexpr auto inverted = [](const UserInput& input) -> UserInput {
            return std::visit([](auto state) -> UserInput {
                state.action = trc::InputAction::release;
                return state;
            }, input.input);
        };

        auto state = ctx.pushFrame(State{ .app=app });
        state.on(inverted(ctx.getProvokingInput()), &State::exitFrame);
        state.onCursorMove([](State& state, const CursorMovement& cursor) {
            const vec2 windowSize = state.app->getTorch().getWindow().getWindowSize();
            const auto diff = cursor.offset / windowSize * state.kDragSpeed;

            const auto invView = glm::inverse(state.app->getScene().getCamera().getViewMatrix());
            const vec4 move = invView * vec4(diff.x, -diff.y, 0, 0);

            state.app->getScene().getCameraArm().translate(move.x, move.y, move.z);
        });
    }

private:
    App* app;
};

class CameraRotateCommand : public Command
{
public:
    struct State : public InputFrame
    {
        App* app;
    };

    explicit CameraRotateCommand(App& app) : app(&app) {}

    void execute(CommandExecutionContext& ctx) override
    {
        constexpr auto invert = [](const UserInput& input) -> UserInput {
            return std::visit([](auto state) -> UserInput {
                state.action = trc::InputAction::release;
                return state;
            }, input.input);
        };

        auto state = ctx.pushFrame(State{ .app=app });
        state.on(invert(ctx.getProvokingInput()), &State::exitFrame);
        state.onCursorMove([](State& state, const CursorMovement& cursor) {
            const vec2 windowSize = state.app->getTorch().getWindow().getWindowSize();
            const vec2 angle = cursor.offset / windowSize * glm::two_pi<float>();
            state.app->getScene().getCameraArm().rotate(-angle.y, angle.x);
        });
    }

private:
    App* app;
};
