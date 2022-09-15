#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace trc::data
{
    namespace internal
    {
        /// Calculate the sum of the sizes of all elements in a tuple up to the I-th element.
        template<size_t I, typename Tp>
        constexpr size_t elem_size_sum = sizeof(std::tuple_element_t<I, Tp>)
                                         + elem_size_sum<I - 1, Tp>;
        /// Recursion terminator
        template<typename Tp>
        inline constexpr size_t elem_size_sum<0, Tp> = sizeof(std::tuple_element_t<0, Tp>);

        /// Calculate the byte offset of an element in a tuple
        template<size_t I, typename Tp>
        constexpr size_t offset = elem_size_sum<I - 1, Tp>;
        /// Guard
        template<typename Tp>
        inline constexpr size_t offset<0, Tp> = 0;
    } // namespace internal

    /**
     * @brief Storage for optional values, indexed by an enum
     *
     * The enum values/options MUST start at the numerical value 0 and MUST
     * ascend strictly in one-increments.
     */
    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    class PropertyField
    {
    public:
        template<EnumType I>
        using Property = std::tuple_element_t<static_cast<size_t>(I), std::tuple<Ts...>>;

        PropertyField();

        template<EnumType I>
        void set(Property<I> value);

        template<EnumType I>
        bool has() const;
        template<EnumType I>
        auto get() -> Property<I>&;
        template<EnumType I>
        auto get() const -> const Property<I>&;

    private:
        template<EnumType I>
        auto _get_elem() -> Property<I>&;
        template<EnumType I>
        auto _get_elem() const -> const Property<I>&;

        static constexpr size_t totalByteSize{ (sizeof(Ts) + ... + 0) };

        std::vector<bool> existsFlag;
        std::array<std::byte, totalByteSize> properties;
    };

    /**
     * @brief Collection of stacks of properties, indexed by an enum
     *
     * The enum values/options MUST start at the numerical value 0 and MUST
     * ascend strictly in one-increments.
     */
    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    class PropertyStack
    {
    public:
        template<EnumType I>
        using Property = std::tuple_element_t<static_cast<size_t>(I), std::tuple<Ts...>>;

        PropertyStack() = default;
        explicit PropertyStack(const PropertyField<EnumType, Ts...>& initialValues);

        template<EnumType I>
        void push(Property<I> value);
        template<EnumType I>
        void pop();

        template<EnumType I>
        auto top() -> Property<I>&;
        template<EnumType I>
        auto top() const -> const Property<I>&;

    private:
        template<typename T>
        using Stack = std::stack<T, std::vector<T>>;

        template<size_t I>
        void _copy_from(const PropertyField<EnumType, Ts...>& field);

        template<EnumType I>
        auto _get_stack() -> Stack<Property<I>>&;
        template<EnumType I>
        auto _get_stack() const -> const Stack<Property<I>>&;

        std::tuple<Stack<Ts>...> stacks;
    };



    ////////////////////////////////////
    //  PropertyField implementation  //
    ////////////////////////////////////

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    PropertyField<EnumType, Ts...>::PropertyField()
        :
        existsFlag(std::tuple_size_v<std::tuple<Ts...>>, false)
    {
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    void PropertyField<EnumType, Ts...>::set(Property<I> value)
    {
        if (!has<I>()) {
            new (&_get_elem<I>()) Property<I>(std::move(value));
        }
        else {
            _get_elem<I>() = std::move(value);
        }
        existsFlag[static_cast<size_t>(I)] = true;
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    bool PropertyField<EnumType, Ts...>::has() const
    {
        return existsFlag[static_cast<size_t>(I)];
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyField<EnumType, Ts...>::get() -> Property<I>&
    {
        if (!has<I>())
        {
            throw std::out_of_range("Property at index "
                                    + std::to_string(static_cast<size_t>(I))
                                    + " is not set.");
        }

        return _get_elem<I>();
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyField<EnumType, Ts...>::get() const -> const Property<I>&
    {
        if (!has<I>())
        {
            throw std::out_of_range("Property at index "
                                    + std::to_string(static_cast<size_t>(I))
                                    + " is not set.");
        }

        return ((PropertyField<EnumType, Ts...>&)*this)._get_elem<I>();
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyField<EnumType, Ts...>::_get_elem() -> Property<I>&
    {
        constexpr size_t byteOffset = internal::offset<static_cast<size_t>(I), std::tuple<Ts...>>;
        static_assert(byteOffset + sizeof(Property<I>) <= totalByteSize);

        return *reinterpret_cast<Property<I>*>(properties.data() + byteOffset);
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyField<EnumType, Ts...>::_get_elem() const -> const Property<I>&
    {
        constexpr size_t byteOffset = internal::offset<static_cast<size_t>(I), std::tuple<Ts...>>;
        static_assert(byteOffset + sizeof(Property<I>) <= totalByteSize);

        return *reinterpret_cast<const Property<I>*>(properties.data() + byteOffset);
    }



    ////////////////////////////////////
    //  PropertyStack implementation  //
    ////////////////////////////////////

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    PropertyStack<EnumType, Ts...>::PropertyStack(const PropertyField<EnumType, Ts...>& field)
    {
        constexpr size_t size = std::tuple_size_v<std::tuple<Ts...>>;
        if constexpr (size > 0) {
            _copy_from<size - 1>(field);
        }
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<size_t I>
    void PropertyStack<EnumType, Ts...>::_copy_from(const PropertyField<EnumType, Ts...>& field)
    {
        constexpr auto enumVal = static_cast<EnumType>(I);
        if (field.template has<enumVal>()) {
            push<enumVal>(field.template get<enumVal>());
        }
        if constexpr (I > 0) {
            _copy_from<I - 1>(field);
        }
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    void PropertyStack<EnumType, Ts...>::push(Property<I> value)
    {
        _get_stack<I>().push(std::move(value));
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    void PropertyStack<EnumType, Ts...>::pop()
    {
        if (_get_stack<I>().empty()) {
            throw std::out_of_range("Stack is empty!");
        }
        _get_stack<I>().pop();
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyStack<EnumType, Ts...>::top() -> Property<I>&
    {
        if (_get_stack<I>().empty()) {
            throw std::out_of_range("Stack is empty!");
        }
        return _get_stack<I>().top();
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyStack<EnumType, Ts...>::top() const -> const Property<I>&
    {
        if (_get_stack<I>().empty()) {
            throw std::out_of_range("Stack is empty!");
        }
        return _get_stack<I>().top();
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyStack<EnumType, Ts...>::_get_stack() -> Stack<Property<I>>&
    {
        return std::get<static_cast<size_t>(I)>(stacks);
    }

    template<typename EnumType, typename ...Ts>
        requires std::is_enum_v<EnumType>
    template<EnumType I>
    auto PropertyStack<EnumType, Ts...>::_get_stack() const -> const Stack<Property<I>>&
    {
        return std::get<static_cast<size_t>(I)>(stacks);
    }
} // namespace trc::data
