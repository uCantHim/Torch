#pragma once

#include <cassert>

#include <concepts>
#include <mutex>
#include <source_location>
#include <stdexcept>
#include <string>
#include <vector>

#include "IdPool.h"
#include "TypesafeId.h"

namespace trc::data
{
    /**
     * @brief Store a value externally and access it by an ID
     *
     * Useful to store small amounts of read-only state (e.g. a model
     * matrix or a few state-indicating integers). Data can only be read
     * by value, never by pointer.
     *
     * Share the ID returned by ExternalStorage<>::getDataId with other
     * objects to provide read-only access. Only the data's owner (i.e.
     * the original `ExternalStorage<>` object) has write access.
     *
     * # Example
     *
     * ```cpp
     * struct MyData { int a; float b; };
     *
     * ExternalStorage<MyData> data;
     * data.set({ .a=42, .b=2.71828f });
     *
     * const auto ref = data.getDataId();
     *
     * assert(data.get().a == 42);
     * assert(ref.get().a == 42);
     * assert(ref.get().b == data.get().b);
     * ```
     */
    template<std::semiregular T>
    class ExternalStorage
    {
    public:
        /**
         * @brief Read-only reference to an ExternalStorage<> object's value
         *
         * Default-constructible to a NONE value.
         */
        struct ID : data::TypesafeID<T, uint32_t>
        {
        private:
            friend class ExternalStorage;
            explicit ID(uint32_t id) : data::TypesafeID<T, uint32_t>(id) {}

        public:
            /**
             * @brief Construct a NONE id
             *
             * `ID::get` will throw if `id == ID::NONE`.
             */
            ID() = default;

            /**
             * @throw std::invalid_argument if `*this == ID::NONE`.
             */
            inline auto get() const -> T {
                return ExternalStorage<T>::getData(*this);
            }
        };

        ExternalStorage() = default;
        ExternalStorage(const ExternalStorage& other);
        ExternalStorage(ExternalStorage&& other) noexcept;
        ~ExternalStorage()
        {
            set(T{});
            staticDataStorage.free(dataId);
        }

        auto operator=(const ExternalStorage& rhs) -> ExternalStorage&;
        auto operator=(ExternalStorage&& rhs) noexcept -> ExternalStorage&;

        inline operator ID() const {
            return dataId;
        }

        /**
         * @brief Retrieve the value referenced by an ID
         *
         * `ExternalStorage<T>::getData(id)` is equivalent to
         * `id.get()`.
         *
         * @throw std::invalid_argument if `id == ID::NONE`.
         */
        static auto getData(ID id) -> T;

        /**
         * @return ID A reference to this object's externally stored value.
         *            IDs grant read-only access.
         */
        inline auto getDataId() const -> ID;

        /**
         * @brief Read the stored value
         *
         * @return T The current value.
         */
        inline auto get() const -> T;

        /**
         * @brief Set the stored value
         *
         * This is the only function that can manipulate the externally stored
         * value. References to the value via `ExternalStorage<>::ID` are
         * read-only.
         *
         * @param const T& value The new value.
         */
        inline void set(const T& value);

        /**
         * @brief Set the stored value
         *
         * This is the only function that can manipulate the externally stored
         * value. References to the value via `ExternalStorage<>::ID` are
         * read-only.
         *
         * @param T&& value The new value.
         */
        inline void set(T&& value);

    private:
        class RegularDataStorage
        {
        public:
            auto create() -> ID;
            void free(ID id);

            auto get(ID id) -> T;
            void set(ID id, const T& value);
            void set(ID id, T&& value);

        private:
            IdPool<uint32_t> idGenerator;
            std::mutex lock;
            std::vector<T> objects;
        };

        static inline RegularDataStorage staticDataStorage;

        ID dataId{ staticDataStorage.create() };
    };

    template<std::semiregular T>
    ExternalStorage<T>::ExternalStorage(
        const ExternalStorage& other)
    {
        set(other.get());
    }

    template<std::semiregular T>
    ExternalStorage<T>::ExternalStorage(ExternalStorage&& other) noexcept
    {
        // Move semantics don't make sense here; just copy the value
        set(other.get());
    }

    template<std::semiregular T>
    auto ExternalStorage<T>::operator=(const ExternalStorage& rhs)
        -> ExternalStorage&
    {
        set(rhs.get());
        return *this;
    }

    template<std::semiregular T>
    auto ExternalStorage<T>::operator=(ExternalStorage&& rhs) noexcept
        -> ExternalStorage&
    {
        // Move semantics don't make sense here; just copy the value
        set(rhs.get());

        return *this;
    }

    template<std::semiregular T>
    auto ExternalStorage<T>::getData(ID id) -> T
    {
        if (id == ID::NONE)
        {
            throw std::invalid_argument(
                "[In " + std::string(std::source_location::current().function_name()) + "]:"
                " ID is NONE!"
            );
        }

        return staticDataStorage.get(id);
    }

    template<std::semiregular T>
    inline auto ExternalStorage<T>::getDataId() const -> ID
    {
        assert(dataId != ID::NONE);
        return dataId;
    }

    template<std::semiregular T>
    inline auto ExternalStorage<T>::get() const -> T
    {
        assert(dataId != ID::NONE);
        return getData(dataId);
    }

    template<std::semiregular T>
    inline void ExternalStorage<T>::set(const T& value)
    {
        assert(dataId != ID::NONE);
        staticDataStorage.set(dataId, value);
    }

    template<std::semiregular T>
    inline void ExternalStorage<T>::set(T&& value)
    {
        assert(dataId != ID::NONE);
        staticDataStorage.set(dataId, std::move(value));
    }



    template<std::semiregular T>
    auto ExternalStorage<T>::RegularDataStorage::create() -> ID
    {
        const uint32_t id = idGenerator.generate();
        if (objects.size() <= id)
        {
            std::scoped_lock lk(lock);
            objects.resize(id + 1);
        }

        objects[id] = {};

        return ID(id);
    }

    template<std::semiregular T>
    void ExternalStorage<T>::RegularDataStorage::free(ID id)
    {
        assert(id != ID::NONE);
        idGenerator.free(id);
    }

    template<std::semiregular T>
    auto ExternalStorage<T>::RegularDataStorage::get(ID id) -> T
    {
        assert(id != ID::NONE);
        assert(objects.size() > id);
        return objects[id];
    }

    template<std::semiregular T>
    void ExternalStorage<T>::RegularDataStorage::set(ID id, const T& value)
    {
        assert(id != ID::NONE);
        assert(objects.size() > id);
        objects[id] = value;
    }

    template<std::semiregular T>
    void ExternalStorage<T>::RegularDataStorage::set(ID id, T&& value)
    {
        assert(id != ID::NONE);
        assert(objects.size() > id);
        objects[id] = std::move(value);
    }
} // namespace trc::data
