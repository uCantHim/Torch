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

        explicit State(App& app) : app(&app) {}

        void onTick(float) override {}
        void onExit() override {}

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

        auto state = ctx.pushFrame(std::make_unique<State>(*app));
        state.on(inverted(ctx.getProvokingInput()), &State::exitFrame);
        state.onCursorMove([](State& state, const CursorMovement& cursor) {
            const vec2 windowSize = state.app->getTorch().getWindow().getWindowSize();
            const auto diff = cursor.offset / windowSize * state.kDragSpeed;

            auto& camera = state.app->getScene().getCameraViewNode();
            camera.translate(diff.x, -diff.y, 0);
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
        explicit State(App& app) : app(&app) {}

        void onTick(float) override {}
        void onExit() override {}

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

        auto state = ctx.pushFrame(std::make_unique<State>(*app));
        state.on(invert(ctx.getProvokingInput()), &State::exitFrame);
        state.onCursorMove([](State& state, const CursorMovement& cursor) {
            const auto diff = cursor.offset;
            const float length = glm::sign(diff.x) * glm::length(diff);

            const vec2 windowSize = state.app->getTorch().getWindow().getWindowSize();
            const float angle = length / windowSize.x * glm::two_pi<float>();

            auto& camera = state.app->getScene().getCamera();
            camera.rotate(0.0f, angle, 0.0f);
        });
    }

private:
    App* app;
};
