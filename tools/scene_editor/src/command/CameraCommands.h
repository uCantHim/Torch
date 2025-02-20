#pragma once

#include "Scene.h"
#include "input/Command.h"
#include "input/InputState.h"

class CameraMoveCommand : public Command
{
public:
    struct State : public InputFrame
    {
        static constexpr float kDragSpeed{ 5.0f };
        Scene& scene;
    };

    explicit CameraMoveCommand(Scene& scene) : scene(scene) {}

    void execute(CommandExecutionContext& ctx) override
    {
        constexpr auto inverted = [](const UserInput& input) -> UserInput {
            return std::visit([](auto state) -> UserInput {
                state.action = trc::InputAction::release;
                return state;
            }, input.input);
        };

        auto state = ctx.pushFrame(State{ .scene=scene });
        state.on(inverted(ctx.getProvokingInput()), &State::exitFrame);
        state.onCursorMove([](State& state, const CursorMovement& cursor) {
            const auto diff = cursor.offset / vec2{cursor.areaSize} * state.kDragSpeed;

            //state.scene.getCamera().unproject(
            const auto invView = glm::inverse(state.scene.getCamera().getViewMatrix());
            const vec4 move = invView * vec4(diff.x, -diff.y, 0, 0);

            state.scene.getCameraArm().translate(move.x, move.y, move.z);
        });
    }

private:
    Scene& scene;
};

class CameraRotateCommand : public Command
{
public:
    struct State : public InputFrame
    {
        Scene& scene;
    };

    explicit CameraRotateCommand(Scene& scene) : scene(scene) {}

    void execute(CommandExecutionContext& ctx) override
    {
        constexpr auto invert = [](const UserInput& input) -> UserInput {
            return std::visit([](auto state) -> UserInput {
                state.action = trc::InputAction::release;
                return state;
            }, input.input);
        };

        auto state = ctx.pushFrame(State{ .scene=scene });
        state.on(invert(ctx.getProvokingInput()), &State::exitFrame);
        state.onCursorMove([](State& state, const CursorMovement& cursor) {
            const vec2 angle = cursor.offset / vec2{cursor.areaSize} * glm::two_pi<float>();
            state.scene.getCameraArm().rotate(-angle.y, angle.x);
        });
    }

private:
    Scene& scene;
};
