#pragma once

#include <functional>
#include <ranges>
#include <unordered_map>

namespace trc::data
{
    /**
     * @brief A set that counts copies of each key.
     */
    template<
        typename Key,
        typename Count = size_t,
        typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>
    >
    class Multiset
    {
    private:
        using MapType = std::unordered_map<Key, Count, Hash, KeyEqual>;

    public:
        using value_type = Key;
        using key_type = Key;

        using size_type = size_t;

        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using const_iterator = typename MapType::const_iterator;

        Multiset(const Multiset&) = default;
        Multiset(Multiset&&) noexcept = default;
        Multiset& operator=(const Multiset&) = default;
        Multiset& operator=(Multiset&&) noexcept = default;
        ~Multiset() noexcept = default;

        Multiset() = default;
        Multiset(std::initializer_list<key_type> init)
        {
            for (const auto& val : init) {
                emplace(val);
            }
        }

        template<std::ranges::input_range R>
        Multiset(std::from_range_t, R&& range)
        {
            for (const auto& val : range) {
                emplace(val);
            }
        }

        auto operator<=>(const Multiset&) const = default;

        /**
         * @return The number of unique elements in the set.
         */
        auto size() const -> size_type {
            return map.size();
        }

        /**
         * @brief Erase all elements from the set.
         */
        void clear() {
            map.clear();
        }

        /**
         * @return The number of duplicates a key has in the set.
         */
        auto count(const key_type& key) const noexcept -> Count
        {
            auto it = map.find(key);
            if (it == map.end()) {
                return 0;
            }
            return it->second;
        }

        /**
         * @brief Test whether a key exists at least once in the set.
         *
         * Equivalent to `count(key) != 0`.
         *
         * @return True if the key exists at least once in the set, false
         *         otherwise.
         */
        bool contains(const key_type& key) const noexcept
        {
            return map.contains(key);
        }

        /**
         * @return The new number of duplicates of the inserted key.
         */
        auto emplace(const key_type& key) -> Count
        {
            auto [it, _] = map.try_emplace(key, 0);
            return ++it->second;
        }

        /**
         * @return The new number of duplicates of the erased key.
         */
        auto erase(const key_type& key) -> Count
        {
            auto it = map.find(key);
            if (it == map.end()) {
                return 0;
            }

            const auto newCount = --it->second;
            if (newCount == 0) {
                map.erase(it);
            }
            return newCount;
        }

        /**
         * @brief Erase all duplicates of a key from the set.
         *
         * After this operation: `count(key) == 0`.
         */
        void erase_all(const key_type& key)
        {
            map.erase(key);
        }

        auto begin() const -> const_iterator {
            return map.begin();
        }
        auto end() const -> const_iterator {
            return map.end();
        }

    private:
        MapType map;
    };
} // namespace trc::util
