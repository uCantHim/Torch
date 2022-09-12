#include "ExternalStorage.h"



template<std::semiregular T>
auto trc::data::ExternalStorage<T>::getData(ID id) -> T
{
    return staticDataStorage.get(id);
}



template<std::semiregular T>
trc::data::ExternalStorage<T>::ExternalStorage(
    const ExternalStorage& other)
{
    set(other.get());
}

template<std::semiregular T>
trc::data::ExternalStorage<T>::ExternalStorage(
    ExternalStorage&& other) noexcept
{
    std::swap(dataId, other.dataId);
}

template<std::semiregular T>
auto trc::data::ExternalStorage<T>::operator=(const ExternalStorage& rhs)
    -> ExternalStorage&
{
    set(rhs.get());
    return *this;
}

template<std::semiregular T>
auto trc::data::ExternalStorage<T>::operator=(ExternalStorage&& rhs) noexcept
    -> ExternalStorage&
{
    std::swap(dataId, rhs.dataId);
    return *this;
}

template<std::semiregular T>
inline auto trc::data::ExternalStorage<T>::getDataId() const -> ID
{
    return dataId;
}

template<std::semiregular T>
inline auto trc::data::ExternalStorage<T>::get() const -> T
{
    return getData(dataId);
}

template<std::semiregular T>
inline void trc::data::ExternalStorage<T>::set(T value)
{
    staticDataStorage.set(dataId, std::move(value));
}



template<std::semiregular T>
auto trc::data::ExternalStorage<T>::RegularDataStorage::create() -> ID
{
    const ui32 id = ui32(idGenerator.generate());
    if (objects.size() <= id)
    {
        std::lock_guard lk(lock);
        objects.resize(id + 1);
    }

    objects[id] = {};

    return ID(id);
}

template<std::semiregular T>
void trc::data::ExternalStorage<T>::RegularDataStorage::free(ID id)
{
    idGenerator.free(id);
}

template<std::semiregular T>
auto trc::data::ExternalStorage<T>::RegularDataStorage::get(ID id) -> T
{
    assert(objects.size() > id);
    return objects[id];
}

template<std::semiregular T>
void trc::data::ExternalStorage<T>::RegularDataStorage::set(ID id, T value)
{
    assert(objects.size() > id);
    objects[id] = value;
}
