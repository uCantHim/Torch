#include "SelfManagedObject.h"

#include <stdexcept>
#include <string>



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
    if (objects.size() > index && objects[index] != nullptr) {
        throw std::runtime_error("Index " + std::to_string(index) + " already occupied");
    }

    auto& result = *objects.emplace(index, new Derived(std::forward<ConstructArgs>(args)...));
    result.myId = index;

    return result;
}

template<class Derived>
auto SelfManagedObject<Derived>::at(ID index) -> Derived&
{
    if (objects.size() <= index || objects[index] == nullptr) {
        throw std::out_of_range("No object at index " + std::to_string(index));
    }

    return *objects.at(index);
}

template<class Derived>
void SelfManagedObject<Derived>::destroy(ID index)
{
    if (objects.size() <= index || objects[index] == nullptr) {
        throw std::out_of_range("No object at index " + std::to_string(index));
    }

    objects.at(index) = std::make_unique<Derived>();
}

template<class Derived>
auto SelfManagedObject<Derived>::id() const noexcept -> ID
{
    return myId;
}
