#include "ObjectPool.h"



template<class T>
inline ObjectPool<T>::ObjectPool(const size_t poolSize)
    :
    objects(poolSize)
{
    for (size_t i = 0; i < poolSize; i++)
    {
        objects[i].id = i;
        objects[i].owningPool = this;
    }
}

template<class T>
inline void ObjectPool<T>::foreachActive(std::function<void(T&)> f)
{
    for (auto& obj : objects) {
        if (obj.isAlive()) {
            f(obj);
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
    owningPool->releaseObject(static_cast<Derived&>(*this));
}
