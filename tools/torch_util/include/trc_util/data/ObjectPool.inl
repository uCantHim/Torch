#include "ObjectPool.h"



template<class T>
inline ObjectPool<T>::ObjectPool(const size_t poolSize)
    :
    objects(poolSize)
{
    for (size_t i = 0; i < poolSize; i++)
    {
        objects[i].alive = false;
        if (i != poolSize - 1) {
            objects[i].poolState.next = &objects[i + 1];
        }
    }
    objects.back().poolState.next = nullptr;
    firstFreeObject = &objects.front();
}

template<class T>
template<typename ...Args>
inline auto ObjectPool<T>::createObject(Args&&... args) -> T&
    requires requires (T a, Args&&... args) { a.poolInit(std::forward<Args>(args)...); }
{
    T& result = createObject();
    result.poolInit(std::forward<Args>(args)...);

    return result;
}

template<class T>
inline auto ObjectPool<T>::createObject() -> T&
{
    T* newObj = firstFreeObject;
    if (newObj == nullptr) {
        throw ObjectPoolOverflowError();
    }

    firstFreeObject = newObj->poolState.next;
    newObj->alive = true;
    newObj->poolState.owningPool = this;

    return *newObj;
}

template<class T>
inline void ObjectPool<T>::releaseObject(T& object)
    requires requires (T a) { a.poolDestroy(); }
{
    doReleaseObject(object);
    object.poolDestroy();
}

template<class T>
inline void ObjectPool<T>::releaseObject(T& object)
{
    doReleaseObject(object);
}

template<class T>
inline void ObjectPool<T>::doReleaseObject(T& object)
{
    assert(object.isAlive());
    assert(object.poolState.owningPool == this);

    object.alive = false;
    object.poolState.next = firstFreeObject;
    firstFreeObject = &object;
}

template<typename T>
template<std::invocable<T&> F>
inline void ObjectPool<T>::foreachActive(F func)
{
    for (auto& obj : objects) {
        if (obj.isAlive()) {
            func(obj);
        }
    }
}



template<class Derived>
inline bool PooledObject<Derived>::isAlive()
{
    return alive;
}

template<class Derived>
inline void PooledObject<Derived>::release()
{
    poolState.owningPool->releaseObject(static_cast<Derived&>(*this));
}
