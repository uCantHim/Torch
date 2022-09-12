#pragma once

#include <type_traits>
#include <vector>
#include <mutex>

#include "TypesafeId.h"
#include "ObjectId.h"

namespace trc::data
{
    /**
     * @brief Store a value externally and access it by an ID
     *
     * Useful to store small amounts of read-only state (e.g. a model
     * matrix or a few state-indicating intergers). The data has read-only
     * access with writes only being possible for its owner. Data can only
     * be read by value, never by pointer.
     *
     * Share the ID returned by ExternalStorage<>::getDataId with other
     * objects to provide read-only access.
     */
    template<std::semiregular T>
    class ExternalStorage
    {
    public:
        struct ID : data::TypesafeID<T, uint32_t>
        {
        private:
            friend class ExternalStorage;
            explicit ID(uint32_t id) : data::TypesafeID<T, uint32_t>(id) {}

        public:
            ID() = default;

            inline auto get() const -> T {
                return ExternalStorage<T>::getData(*this);
            }
        };

        ExternalStorage() = default;
        ExternalStorage(const ExternalStorage& other);
        ExternalStorage(ExternalStorage&& other) noexcept;
        ~ExternalStorage() {
            set(T{});
            staticDataStorage.free(dataId);
        }

        auto operator=(const ExternalStorage& rhs) -> ExternalStorage&;
        auto operator=(ExternalStorage&& rhs) noexcept -> ExternalStorage&;

        inline operator ID() const {
            return dataId;
        }

        static auto getData(ID id) -> T;

        inline auto getDataId() const -> ID;

        inline auto get() const -> T;
        inline void set(T value);

    private:
        class RegularDataStorage
        {
        public:
            auto create() -> ID;
            void free(ID id);

            auto get(ID id) -> T;
            void set(ID id, T value);

        private:
            IdPool idGenerator;
            std::mutex lock;
            std::vector<T> objects;
        };

        static inline RegularDataStorage staticDataStorage;

        ID dataId{ staticDataStorage.create() };
    };

#include "ExternalStorage.inl"

} // namespace trc::data
