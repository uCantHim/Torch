#pragma once

#include <cassert>

#include <concepts>
#include <initializer_list>
#include <vector>

namespace trc::util
{
    /**
     * @brief An adapter to a fixed-size container.
     */
    template<
        typename T,
        typename Container = std::vector<T>
    >
    class FixedSize
    {
    public:
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using size_type = Container::size_type;

        using iterator = Container::iterator;
        using const_iterator = Container::const_iterator;

        FixedSize() = delete;

        FixedSize(const FixedSize&) = default;
        FixedSize(FixedSize&&) noexcept = default;
        FixedSize& operator=(const FixedSize&) = default;
        FixedSize& operator=(FixedSize&&) noexcept = default;
        ~FixedSize() noexcept = default;

        /**
         * Initialize the container to a fixed size and default construct its
         * `size` elements.
         */
        explicit
        FixedSize(size_type size)
            requires std::default_initializable<value_type>
            : container(size)
        {}

        /**
         * Initialize the container to a fixed size and initialize its elements
         * to a specific initial value.
         */
        FixedSize(size_type size, const value_type& initialValue)
            : container{ size, initialValue }
        {}

        /**
         * Construct the container and its elements from an initializer list.
         */
        FixedSize(std::initializer_list<value_type> init)
            : container{ std::move(init) }
        {}

        /**
         * Construct the adapter from the underlying container type.
         */
        explicit FixedSize(Container init)
            : container(std::move(init))
        {}

        /**
         * Construct the container from a range of elements.
         */
        template<std::ranges::input_range R>
        FixedSize(std::from_range_t, R&& range)
            : container(std::ranges::begin(range), std::ranges::end(range))
        {}

        /**
         * Assign to the adapter from the underlying container type.
         */
        auto operator=(Container cont) -> FixedSize&
        {
            container = std::move(cont);
            return *this;
        }

        /**
         * Compare the container to another container. Uses the underlying
         * container's comparison semantics.
         */
        auto operator<=>(const FixedSize&) const = default;

        /**
         * Compare the container to an instance of the underlying container
         * type. Uses the underlying container's comparison semantics.
         */
        auto operator<=>(const Container& other) const {
            return this->container <=> other;
        }

        /**
         * @return The container's size. Is constant across the container's
         *         lifetime.
         */
        auto size() const -> size_type {
            return container.size();
        }

        /**
         * @brief Access an element in the container.
         */
        auto at(size_type i) -> reference {
            return container.at(i);
        }

        /**
         * @brief Access an element in the container.
         */
        auto at(size_type i) const -> const_reference {
            return container.at(i);
        }

        /**
         * @brief Access an element in the container.
         */
        auto operator[](size_type i) -> reference {
            assert(i < this->size());
            return container[i];
        }

        /**
         * @brief Access an element in the container.
         */
        auto operator[](size_type i) const -> const_reference {
            assert(i < this->size());
            return container[i];
        }

        auto begin() -> iterator {
            return container.begin();
        }

        auto end() -> iterator {
            return container.end();
        }

        auto begin() const -> const_iterator {
            return container.begin();
        }

        auto end() const -> const_iterator {
            return container.end();
        }

    private:
        Container container;
    };

    /** Deduction guide for std::ranges::to */
    template<std::ranges::input_range R>
    FixedSize(std::from_range_t, R&&) -> FixedSize<std::ranges::range_value_t<R>>;
} // namespace trc::util
