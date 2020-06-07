#pragma once

#include <memory>
#include <type_traits>
#include <atomic>

template<typename T>
class ObjectHandle;

/**
 */
template<typename T>
class SortedObjectPool
{
public:
    using HandleType = ObjectHandle<T>;

private:

};

/**
 */
template<typename T>
class ObjectHandle
{
private:
    using Owner = SortedObjectPool<T>;

    friend Owner;
    ObjectHandle(Owner& owner);

public:
    ~ObjectHandle();

    ObjectHandle(const ObjectHandle<T>& other);
    ObjectHandle(ObjectHandle<T>&& other) noexcept;

    ObjectHandle<T>& operator=(const ObjectHandle<T>& rhs);
    ObjectHandle<T>& operator=(ObjectHandle<T>&& rhs) noexcept;

    auto operator*() noexcept -> T&;
    auto operator*() const noexcept -> const T&;

    auto operator->() noexcept -> T*;
    auto operator->() const noexcept -> const T*;

private:
    Owner* owner;
    uint32_t* id;

    T* object;
    std::shared_ptr<std::atomic<uint32_t>> referenceCount;
};
