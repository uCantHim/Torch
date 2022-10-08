#pragma once

#include <array>

#include <glm/glm.hpp>

#include "trc/base/event/Keys.h"

namespace trc
{
    /**
     * @brief Static keyboard state
     */
    class Keyboard
    {
    public:
        /**
         * @brief Register necessary event handlers
         *
         * The overhead of this class should be negligible, but you can
         * still choose if you want to use it or not.
         */
        static void init();

        static bool isPressed(Key key);
        static bool isReleased(Key key);

        static auto getState(Key key) -> InputAction;

    private:
        static inline std::array<InputAction, static_cast<size_t>(Key::MAX_ENUM)> states{
            InputAction::release
        };
    };

    /**
     * @brief Static mouse state
     */
    class Mouse
    {
    public:
        /**
         * @brief Register necessary event handlers
         *
         * The overhead of this class should be negligible, but you can
         * still choose if you want to use it or not.
         */
        static void init();

        static bool isPressed(MouseButton button);
        static bool isReleased(MouseButton button);

        static auto getState(MouseButton button) -> InputAction;
        static auto getPosition() -> glm::vec2;

    private:
        static inline std::array<InputAction, static_cast<size_t>(MouseButton::MAX_ENUM)> states{
            InputAction::release
        };

        static inline glm::vec2 mousePos;
    };
} // namespace trc
