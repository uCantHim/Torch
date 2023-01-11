#pragma once

#include <trc/base/event/InputState.h>

#include "App.h"
#include "input/InputCommand.h"
#include "input/InputState.h"

class CameraMoveCommand : public InputCommand
{
public:
    struct State : public CommandState
    {
        static constexpr float kDragSpeed{ 5.0f };

        explicit State(App& app) : app(&app) {}

        bool update(float) override
        {
            const auto now = trc::Mouse::getPosition();
            const vec2 windowSize = app->getTorch().getWindow().getWindowSize();
            const auto diff = (now - prevMousePos) / windowSize * kDragSpeed;

            auto& camera = app->getScene().getCamera();
            camera.translate(glm::inverse(camera.getViewMatrix()) * vec4(diff.x, -diff.y, 0, 0));

            prevMousePos = now;

            return exit;
        }

        void onExit() override {}

        App* app;
        vec2 prevMousePos{ trc::Mouse::getPosition() };
        bool exit{ false };
    };

    explicit CameraMoveCommand(App& app) : app(&app) {}

    void execute(CommandCall& call) override
    {
        constexpr auto invert = [](const VariantInput& input) -> VariantInput {
            return std::visit([](auto state) -> VariantInput {
                state.action = trc::InputAction::release;
                return state;
            }, input.input);
        };

        auto state = std::make_unique<State>(*app);

        call.on(invert(call.getProvokingInput()),
                [state=state.get()](auto&){ state->exit = true; });

        call.setState(std::move(state));
    }

private:
    App* app;
};

class CameraRotateCommand : public InputCommand
{
public:
    struct State : public CommandState
    {
        explicit State(App& app) : app(&app) {}

        bool update(float) override
        {
            const auto now = trc::Mouse::getPosition();
            const auto diff = now - prevMousePos;
            const float length = glm::sign(diff.x) * glm::length(diff);

            const vec2 windowSize = app->getTorch().getWindow().getWindowSize();
            const float angle = length / windowSize.x * glm::two_pi<float>();

            auto& camera = app->getScene().getCamera();
            camera.rotate(0.0f, angle, 0.0f);

            prevMousePos = now;

            return exit;
        }

        void onExit() override {}

        App* app;
        vec2 prevMousePos{ trc::Mouse::getPosition() };
        bool exit{ false };
    };

    explicit CameraRotateCommand(App& app) : app(&app) {}

    void execute(CommandCall& call) override
    {
        constexpr auto invert = [](const VariantInput& input) -> VariantInput {
            return std::visit([](auto state) -> VariantInput {
                state.action = trc::InputAction::release;
                return state;
            }, input.input);
        };

        auto state = std::make_unique<State>(*app);

        call.on(invert(call.getProvokingInput()),
                [state=state.get()](auto&){ state->exit = true; });

        call.setState(std::move(state));
    }

private:
    App* app;
};
