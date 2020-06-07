#include "ObjectPool.h"



template<class T>
inline datautil::ObjectPool<T>::ObjectPool(const size_t poolSize)
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
inline void datautil::ObjectPool<T>::foreachActive(std::function<void(T&)> f)
{
    for (auto& obj : objects) {
        if (obj.isAlive()) {
            f(obj);
        }
    }
}



template<class Derived>
inline bool datautil::PooledObject<Derived>::isAlive()
{
    return alive;
}

template<class Derived>
inline void datautil::PooledObject<Derived>::release()
{
    owningPool->releaseObject(static_cast<Derived&>(*this));
}
