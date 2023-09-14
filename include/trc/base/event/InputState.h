#pragma once

#include <array>
#include <bitset>

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

        /**
         * @return bool True if the key was pressed at any time between this
         *              and the last call to `wasPressed`.
         */
        static bool wasPressed(Key key);

        /**
         * @return bool True if the key was released at any time between this
         *              and the last call to `wasReleased`.
         */
        static bool wasReleased(Key key);

        static auto getState(Key key) -> InputAction;

    private:
        static constexpr size_t kNumKeys = static_cast<size_t>(Key::MAX_ENUM);
        static inline std::array<InputAction, kNumKeys> states{ InputAction::release };
        static inline std::bitset<kNumKeys> firstTimePressed;
        static inline std::bitset<kNumKeys> firstTimeReleased;
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

        /**
         * @return bool True if the button was pressed at any time between this
         *              and the last call to `wasPressed`.
         */
        static bool wasPressed(MouseButton button);

        static bool isReleased(MouseButton button);

        /**
         * @return bool True if the button was released at any time between this
         *              and the last call to `wasReleased`.
         */
        static bool wasReleased(MouseButton button);

        static auto getState(MouseButton button) -> InputAction;

        static auto getPosition() -> glm::vec2;

        /**
         * @return bool True if the mouse was moved between this and the last
         *              call to `wasMoved`.
         */
        static bool wasMoved();

    private:
        static constexpr size_t kNumButtons = static_cast<size_t>(MouseButton::MAX_ENUM);

        static inline std::array<InputAction, kNumButtons> states{ InputAction::release };
        static inline std::bitset<kNumButtons> firstTimePressed;
        static inline std::bitset<kNumButtons> firstTimeReleased;

        static inline glm::vec2 mousePos;
        static inline bool firstTimeMoved{ false };
    };
} // namespace trc
