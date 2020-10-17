#pragma once

#include <memory>
#include <concepts>

#include "Exception.h"

namespace trc
{
    class MaybeEmptyError : public Exception
    {
    public:
        MaybeEmptyError() : Exception("Tried to access Maybe, but it was empty!") {}
    };

    /**
     * @brief A one-time use, very fancy std::optional
     */
    template<typename T>
    class Maybe
    {
    public:
        /**
         * @brief Construct an empty Maybe value
         */
        Maybe() = default;

        /**
         * @brief Construct Maybe from an lvalue
         */
        Maybe(const T& val) requires std::is_copy_constructible_v<T>
            : value(val) {}

        /**
         * @brief Construct Maybe from an rvalue
         */
        Maybe(T&& val) requires std::is_move_constructible_v<T>
            : value(std::move(val)) {}

        /**
         * @brief Constuctor Maybe from a std::optional
         */
        explicit Maybe(std::optional<T>&& opt)
            : value(std::move(opt)) {}

        ~Maybe() noexcept = default;

        // Defaulted move constructor
        Maybe(Maybe<T>&&) noexcept = default;
        // Defaulted move assignment operator
        auto operator=(Maybe<T>&&) noexcept -> Maybe<T>& = default;

        // Deleted copy constructor
        Maybe(const Maybe<T>&) = delete;
        // Deleted copy assignment operator
        auto operator=(const Maybe<T>&) -> Maybe<T> = delete;

        /**
         * Pipe operator that puts the value into a variable
         *
         * @return T& Reference to the variable that the value was stored
         *            in (i.e. the right-hand operand). This allows for
         *            code like this:
         *
         *                (m >> i) += 4;
         *
         * @throw MaybeEmptyError if the Maybe does not have a value.
         */
        inline auto operator>>(T& rhs) -> T&
        {
            rhs = getValue();
            return rhs;
        }

        /*
         * @brief Invoke a function with the value contained in the Maybe
         *
         * @return The value returned by the function.
         *
         * @throw MaybeEmptyError if the Maybe does not have a value.
         */
        template<typename Func>
        inline auto operator>>(Func& rhs) -> decltype(std::declval<Func>()(std::declval<T>()))
            requires requires(T val, Func func) { func(val); }
        {
            return rhs(getValue());
        }

        /**
         * value_or operator
         */
        inline auto operator||(T&& rhs) -> T
        {
            if (hasValue()) {
                return getValue();
            }
            else {
                return rhs;
            }
        }

        /**
         * value_or with a function argument for lazy evaluation
         *
         * @return The Maybe's value if one is present or the function's
         *         return value if there isn't.
         */
        template<typename Func>
        inline auto operator||(Func rhs) -> T
            requires std::invocable<Func>
                  && requires (Func func) { std::is_same_v<T, decltype(func())>; }
        {
            if (hasValue()) {
                return getValue();
            }
            else {
                return rhs();
            }
        }

        /**
         * @brief Invoke one of two function based on emptyness of the Maybe
         *
         * Invokes the first function with the Maybe's value if it has a
         * value. Invokes the second function if it doesn't.
         *
         * @return The return value of the invoked function.
         */
        template<typename Success, typename Error>
        inline auto maybe(Success success, Error error) -> decltype(std::declval<Error>()())
            requires std::invocable<Success, T>
                  && std::invocable<Error>
                  && requires (T val, Success s, Error e) {
                         std::is_same_v<decltype(s(val)), decltype(e())>;
                     }
        {
            if (hasValue()) {
                return success(getValue());
            }
            else {
                return error();
            }
        }

        /**
         * @brief Get the value in the Maybe
         *
         * @return T The Maybe's value if it has one.
         *
         * @throw MaybeEmptyError if the Maybe does not have a value.
         */
        inline auto operator*() -> T
        {
            return getValue();
        }

        /**
         * @brief Get the value in the Maybe
         *
         * @return T The Maybe's value if it has one.
         *
         * @throw MaybeEmptyError if the Maybe does not have a value.
         */
        inline auto get() -> T
        {
            return getValue();
        }

        /**
         * @brief Get the stored value or a default value if the Maybe is empty
         *
         * @param T&& fallback The default value retuned in case the Maybe
         *                     is empty.
         *
         * @return T
         */
        inline auto getOr(T&& fallback) -> T
        {
            if (hasValue()) {
                return getValue();
            }
            else {
                return fallback;
            }
        }

    private:
        inline auto getValue() -> T
        {
            if (!hasValue()) {
                throw MaybeEmptyError();
            }

            // Set the Maybe's value to nullopt
            std::optional<T> opt{ std::nullopt };
            value.swap(opt);

            return std::move(*opt);
        }

        inline bool hasValue() const
        {
            return value.has_value();
        }

        std::optional<T> value{ std::nullopt };
    };
} // namespace trc
