#include "SelfManagedObject.h"


template<class Derived>
template<typename ...ConstructArgs>
auto SelfManagedObject<Derived>::createAtNextIndex(ConstructArgs&&... args)
    -> std::pair<ID, std::reference_wrapper<Derived>>
{
    ID nextIndex = objects.size();

    return { nextIndex, create(nextIndex, std::forward<ConstructArgs>(args)...) };
}

template<class Derived>
template<typename ...ConstructArgs>
auto SelfManagedObject<Derived>::create(ID index, ConstructArgs&&... args) -> Derived&
{
    auto& result = objects.emplace(index, std::forward<ConstructArgs>(args)...);
    result.myId = index;

    return result;
}

template<class Derived>
auto SelfManagedObject<Derived>::at(ID index) -> Derived&
{
    return objects.at(index);
}

template<class Derived>
void SelfManagedObject<Derived>::destroy(ID index)
{
    objects.at(index) = {};
}

template<class Derived>
auto SelfManagedObject<Derived>::id() const noexcept -> ID
{
    return myId;
}
