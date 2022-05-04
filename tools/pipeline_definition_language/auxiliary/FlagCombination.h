#pragma once

#include <cstdint>
#include <cstddef>
#include <concepts>
#include <array>
#include <tuple>

namespace se
{
    template<typename T>
    concept FlagBitsType = requires {
        std::is_enum_v<T>;
        T::eMaxEnum;
    };

    /**
     * @brief A combination of different flag types
     *
     * Combines different flag enums of which only one value can be set at
     * any time.
     *
     * Can be evaluated entirely at compile time.
     */
    template<FlagBitsType ...Ts>
    class FlagCombination
    {
        using Tuple = std::tuple<Ts...>;

        template<FlagBitsType...>
        friend class FlagCombination;

        template<typename T, typename TupleType>
        struct ElemIndex;

        template<typename T, typename ...Rest>
        struct ElemIndex<T, std::tuple<T, Rest...>> {
            static constexpr size_t value = 0;
        };

        template<typename T, typename U, typename ...Rest>
        struct ElemIndex<T, std::tuple<U, Rest...>> {
            static constexpr size_t value = 1 + ElemIndex<T, std::tuple<Rest...>>::value;
        };

        template<FlagBitsType T>
        static constexpr size_t staticIndex{ ElemIndex<T, Tuple>::value };

        /** Builds the product of all Ts::eMaxEnum. Used for size() */
        template<typename T, typename ...Types>
        struct TotalMaxEnums {
            static constexpr size_t value = TotalMaxEnums<T>::value * TotalMaxEnums<Types...>::value;
        };

        /** Recursion terminator */
        template<typename T>
        struct TotalMaxEnums<T> {
            static constexpr size_t value = static_cast<size_t>(T::eMaxEnum);
        };

        /**
         * Initialize array elements at compile time
         */
        template<size_t I>
            requires (I < std::tuple_size_v<Tuple>)
        constexpr void _initElems()
        {
            values[I] = FlagValue{ 0, static_cast<uint32_t>(std::tuple_element_t<I, Tuple>::eMaxEnum) };
            if constexpr (I > 0) {
                _initElems<I - 1>();
            }
        }

        /**
         * @brief Copy the I-th element from another FlagCombination type
         */
        template<size_t I, typename ...Us>
            requires (I < std::tuple_size_v<std::tuple<Us...>>)
        constexpr void _copy_element(const FlagCombination<Us...>& other)
        {
            using OtherTuple = std::tuple<Us...>;
            using Element = std::tuple_element_t<I, OtherTuple>;

            const bool contains = (std::same_as<Ts, Element> || ...);
            if constexpr (contains)
            {
                const auto& val = other.values[ElemIndex<Element, OtherTuple>::value];
                values[staticIndex<Element>] = FlagValue{ val.bits, val.maxValue };
            }
        }

        template<size_t I, typename ...Us>
            requires (I < std::tuple_size_v<std::tuple<Us...>>)
        constexpr void _copy(const FlagCombination<Us...>& other)
        {
            _copy_element<I>(other);
            if constexpr (I > 0) {
                _copy<I - 1, Us...>(other);
            }
        }

    public:
        constexpr FlagCombination(const FlagCombination&) = default;
        constexpr FlagCombination(FlagCombination&&) noexcept = default;
        constexpr auto operator=(const FlagCombination&) -> FlagCombination& = default;
        constexpr auto operator=(FlagCombination&&) noexcept -> FlagCombination& = default;
        constexpr ~FlagCombination() = default;

        /** @brief Default constructor */
        constexpr FlagCombination()
        {
            _initElems<std::tuple_size_v<Tuple> - 1>();
        }

        template<typename ...Us>
        constexpr FlagCombination(const FlagCombination<Us...>& other)
        {
            _initElems<std::tuple_size_v<Tuple> - 1>();
            _copy<std::tuple_size_v<std::tuple<Us...>> - 1, Us...>(other);
        }

        /**
         * @brief Implicit conversion from bit type
         */
        template<FlagBitsType T>
        constexpr FlagCombination(T bit)
            : FlagCombination()
        {
            set(bit);
        }

        /**
         * @return size_t Number of possible combinations of all flags.
         *                Equals (maximum index returned by toIndex()) + 1.
         */
        static constexpr auto size() -> size_t
        {
            return TotalMaxEnums<Ts...>::value;
        }

        template<FlagBitsType T>
        constexpr void set(T flagBit)
        {
            values[staticIndex<T>].bits = static_cast<uint32_t>(flagBit);
        }

        template<FlagBitsType T>
        constexpr auto get() const -> T
        {
            return static_cast<T>(values[staticIndex<T>].bits);
        }

        template<FlagBitsType T>
        constexpr bool has(T t) const
        {
            return get<T>() == t;
        }

        constexpr auto toIndex() const -> uint32_t
        {
            uint32_t index{ 0 };
            uint32_t totalMax{ 1 };
            for (auto& flag : values)
            {
                index += totalMax * flag.bits;
                totalMax *= flag.maxValue;
            }
            return index;
        }

    private:
        struct FlagValue
        {
            uint32_t bits{ 0 };
            uint32_t maxValue{ 0 };
        };

        std::array<FlagValue, std::tuple_size_v<Tuple>> values;
    };

    /**
     * @brief Combine two flags of different types
     */
    template<FlagBitsType T, FlagBitsType U>
        requires (!std::same_as<T, U>)
    constexpr auto operator|(T t, U u) -> FlagCombination<T, U>;

    /**
     * @brief Add a new flag type to a flag combination
     */
    template<FlagBitsType ...Ts, FlagBitsType U>
        requires (!(std::same_as<Ts, U> || ...))
    constexpr auto operator|(FlagCombination<Ts...> flags, U u)
        -> FlagCombination<Ts..., U>;

    /**
     * @brief Set a flag bit
     */
    template<FlagBitsType T, FlagBitsType ...Ts>
        requires (std::same_as<Ts, T> || ...)
    constexpr auto operator|(const FlagCombination<Ts...>& flags, T t) -> FlagCombination<Ts...>;

    /**
     * @brief Set a flag bit in-place
     */
    template<FlagBitsType T, FlagBitsType ...Ts>
        requires (std::same_as<Ts, T> || ...)
    constexpr auto operator|=(const FlagCombination<Ts...>& flags, T t) -> FlagCombination<Ts...>&;

    /**
     * @brief Test if a specific flag is set in a flag combination
     */
    template<FlagBitsType T, FlagBitsType ...Ts>
        requires (std::same_as<Ts, T> || ...)
    constexpr bool operator&(const FlagCombination<Ts...>& flags, T t);



    //////////////////////////////////////
    //     Operator Implementations     //
    //////////////////////////////////////

    template<FlagBitsType T, FlagBitsType U>
        requires (!std::same_as<T, U>)
    constexpr auto operator|(T t, U u) -> FlagCombination<T, U>
    {
        FlagCombination<T, U> result;
        result.set(t);
        result.set(u);
        return result;
    }

    /** Helper that copies all flags from the highest index */
    template<FlagBitsType U, FlagBitsType ...Ts>
    constexpr void _copy_flag_combination(FlagCombination<Ts..., U>& dst,
                                          const FlagCombination<Ts...>& src)
    {
        _copy_flag_combination<std::tuple_size_v<std::tuple<Ts...>> - 1, U, Ts...>(dst, src);
    }

    /** Helper that copies flags from a smaller type to a bigger type recursively */
    template<size_t I, FlagBitsType U, FlagBitsType ...Ts>
    constexpr void _copy_flag_combination(FlagCombination<Ts..., U>& dst,
                                          const FlagCombination<Ts...>& src)
    {
        using InputTuple = std::tuple<Ts...>;
        using Element = std::tuple_element_t<I, InputTuple>;

        dst.set(src.template get<Element>());
        if constexpr (I > 0) {
            _copy_flag_combination<I - 1, U, Ts...>(dst, src);
        }
    }

    template<FlagBitsType ...Ts, FlagBitsType U>
        requires (!(std::same_as<Ts, U> || ...))
    constexpr auto operator|(FlagCombination<Ts...> flags, U u)
        -> FlagCombination<Ts..., U>
    {
        FlagCombination<Ts..., U> result;
        _copy_flag_combination<U, Ts...>(result, flags);
        result.set(u);
        return result;
    }

    template<FlagBitsType T, FlagBitsType ...Ts>
        requires (std::same_as<Ts, T> || ...)
    constexpr auto operator|(const FlagCombination<Ts...>& flags, T t) -> FlagCombination<Ts...>
    {
        auto result = flags;
        result.set(t);
        return result;
    }

    template<FlagBitsType T, FlagBitsType ...Ts>
        requires (std::same_as<Ts, T> || ...)
    constexpr auto operator|=(FlagCombination<Ts...>& flags, T t) -> FlagCombination<Ts...>&
    {
        flags.set(t);
        return flags;
    }

    template<FlagBitsType T, FlagBitsType ...Ts>
        requires (std::same_as<Ts, T> || ...)
    constexpr bool operator&(const FlagCombination<Ts...>& flags, T t)
    {
        return flags.has(t);
    }



    inline void foo()
    {
        enum class Foo
        {
            eNone = 0,
            eOne,
            eTwo,
            eMaxEnum,
        };

        enum class Bar
        {
            eNone = 0,
            eFirst = 1,
            eSecond = 2,
            eMaxEnum,
        };

        enum class Baz
        {
            eNone = 0,
            a,
            b,
            c,
            eMaxEnum,
        };

        constexpr FlagCombination<Bar, Baz> a;
        static_assert(a.has(Bar::eNone));
        static_assert(!a.has(Bar::eFirst));
        static_assert(!a.has(Bar::eSecond));
        static_assert(a.toIndex() == 0);
        static_assert(a & Baz::eNone);
        static_assert(!(a & Baz::c));

        constexpr auto flags = Bar::eFirst | Baz::b;
        static_assert(!flags.has(Baz::a));
        static_assert(flags.has(Bar::eFirst));
        static_assert(flags.has(Baz::b));
        static_assert(flags.toIndex() == 7);

        // Copy from smaller type
        constexpr FlagCombination<Bar, Baz, Foo> b(flags);
        static_assert(b.has(Bar::eFirst));
        static_assert(b.has(Baz::b));
        static_assert(b.has(Foo::eNone));

        constexpr auto flags2 = flags | Foo::eTwo;
        static_assert(flags2.has(Bar::eFirst));
        static_assert(flags2.has(Baz::b));
        static_assert(flags2.has(Foo::eTwo));
        static_assert(flags2.toIndex() == 31);

        constexpr auto max = Bar::eSecond | Baz::c;
        static_assert(max.toIndex() == max.size() - 1);
    }
} // namespace trc
