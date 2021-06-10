#pragma once

#include <memory>
#include <concepts>
#include <functional> // for reference_wrapper
#include <optional>

#include "../Exception.h"

namespace nc::functional
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
        using StoredType = std::conditional_t<std::is_lvalue_reference_v<T>,
            std::reference_wrapper<std::remove_reference_t<T>>,
            T
        >;

    public:
        /**
         * @brief Construct an empty Maybe value
         */
        Maybe() = default;

        /**
         * @brief Construct Maybe from an lvalue
         */
        Maybe(T val)
            requires (!std::is_lvalue_reference_v<T>)
            : value(std::move(val)) {}

        /**
         * @brief Construct Maybe from an lvalue reference
         */
        Maybe(T val)
            requires std::is_lvalue_reference_v<T>
                  && (!std::is_const_v<std::remove_reference_t<T>>)
            : value(std::ref(val)) {}

        /**
         * @brief Construct Maybe from a const lvalue reference
         */
        Maybe(T val)
            requires std::is_lvalue_reference_v<T>
                  && std::is_const_v<std::remove_reference_t<T>>
            : value(std::cref(val)) {}

        /**
         * @brief Constuctor Maybe from a std::optional
         */
        explicit Maybe(std::optional<T> opt)
            requires (!std::is_lvalue_reference_v<T>)
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

        /*
         * @brief Monadic function application
         *
         * @return The value returned by the right-hand function or an
         *         empty Maybe.
         */
        template<std::invocable<T> Func>
            requires (!std::is_same_v<std::invoke_result_t<Func, T>, void>)
        inline auto operator>>(Func&& rhs) -> Maybe<std::invoke_result_t<Func, T>>
        {
            if (hasValue()) {
                return { rhs(getValue()) };
            }

            return {}; // Nothing
        }

        /**
         * @brief Function application with no return value
         */
        template<std::invocable<T> Func>
            requires std::is_same_v<std::invoke_result_t<Func, T>, void>
        inline void operator>>(Func&& rhs)
        {
            if (hasValue()) {
                rhs(getValue());
            }
        }

        /**
         * value_or operator
         */
        inline auto operator||(T rhs) -> T
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
        template<std::invocable Func>
        inline auto operator||(Func&& rhs) -> T
            requires requires (Func func) {
                { func() } -> std::same_as<T>;
            }
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
            requires requires () {
                std::invocable<Success, T>;
                std::invocable<Error>;
                std::same_as<
                    std::invoke_result_t<Success, T>,
                    std::invoke_result_t<Error>
                >;
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
            auto result = [this] {
                if constexpr (std::is_lvalue_reference_v<T>) {
                    return value.value();
                }
                else {
                    return std::move(value.value());
                }
            }();
            value = std::nullopt;

            return result;
        }

        inline bool hasValue() const
        {
            return value.has_value();
        }

        std::optional<StoredType> value{ std::nullopt };
    };
} // namespace nc::functional
