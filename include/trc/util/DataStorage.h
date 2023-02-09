#pragma once

#include <filesystem>
#include <istream>
#include <ostream>

#include "trc/util/Pathlet.h"

namespace trc
{
    /**
     * @brief A (key -> value) data storage interface
     */
    class DataStorage
    {
    public:
        using path = util::Pathlet;

        struct iterator;
        using const_iterator = const iterator;

        DataStorage() = default;
        virtual ~DataStorage() = default;

        /**
         * @brief Read data from a location
         *
         * @return s_ptr<std::istream> An input stream. Nullptr if unable
         *         to read data from `path`.
         */
        virtual auto read(const path& path) -> s_ptr<std::istream> = 0;

        /**
         * @brief Write data to a location
         *
         * Overwrites any existing data at `path`.
         *
         * @return s_ptr<std::ostream> An output stream. Nullptr if unable
         *         to write data to `path`.
         */
        virtual auto write(const path& path) -> s_ptr<std::ostream> = 0;

        /**
         * @brief Remove a value from the storage
         *
         * @return bool True if an item at `path` was erased, false if no
         *              value was found at `path`.
         */
        virtual bool remove(const path& path) = 0;

        virtual auto begin() -> iterator {
            return end();
        }

        auto end() -> iterator {
            return iterator{ nullptr };
        }

    protected:
        class EntryIterator;

    public:
        struct iterator
        {
            using value_type = path;
            using reference = path&;
            using const_reference = const path&;
            using pointer = path*;
            using const_pointer = const path*;
            using size_type = size_t;

            inline auto operator*() -> reference { return **iter; };
            inline auto operator*() const -> const_reference { return **iter; };
            inline auto operator->() -> pointer { return iter->operator->(); }
            inline auto operator->() const -> const_pointer { return iter->operator->(); }

            inline auto operator++() -> iterator& { ++*iter; return *this; }
            inline auto operator--() -> iterator& { --*iter; return *this; }

            inline bool operator==(const iterator& other) const { return *iter == *other.iter; }
            inline bool operator!=(const iterator& other) const { return *iter != *other.iter; };

        private:
            friend class DataStorage;

            explicit iterator(u_ptr<EntryIterator> iter) : iter(std::move(iter)) {}
            u_ptr<EntryIterator> iter;
        };

    protected:
        class EntryIterator
        {
        public:
            using value_type = path;
            using reference = path&;
            using const_reference = const path&;
            using pointer = path*;
            using const_pointer = const path*;
            using size_type = size_t;

            virtual ~EntryIterator() noexcept = default;

            virtual auto operator*() -> reference = 0;
            virtual auto operator*() const -> const_reference = 0;
            virtual auto operator->() -> pointer = 0;
            virtual auto operator->() const -> const_pointer = 0;

            // Only define prefix increment/decrement because postfix must
            // return a value type, which is not possible for abstract classes
            virtual auto operator++() -> EntryIterator& = 0;
            virtual auto operator--() -> EntryIterator& = 0;

            virtual bool operator==(const EntryIterator&) const = 0;
            virtual bool operator!=(const EntryIterator&) const = default;
        };
    };
} // namespace trc
