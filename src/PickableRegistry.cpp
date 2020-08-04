#include "PickableRegistry.h"



auto trc::Pickable::getPickableId() const noexcept -> ID
{
    return id;
}

void trc::Pickable::setPickableId(ID pickableId)
{
    id = pickableId;
}



trc::PickableFunctional::PickableFunctional(
    std::function<void()> onPick,
    std::function<void()> onUnpick,
    void* userData)
    :
    onPickCallback(std::move(onPick)),
    onUnpickCallback(std::move(onUnpick)),
    userData(userData)
{
}

void trc::PickableFunctional::onPick()
{
    onPickCallback();
}

void trc::PickableFunctional::onUnpick()
{
    onUnpickCallback();
}



void trc::PickableRegistry::destroyPickable(Pickable& pickable)
{
    assert(pickables.size() > pickable.getPickableId());
    assert(pickables.at(pickable.getPickableId()) != nullptr);

    auto id = pickable.getPickableId();
    pickables[id].reset();
    freeIds.push_back(id);
}

auto trc::PickableRegistry::getPickable(Pickable::ID pickable) -> Pickable&
{
    assert(pickables.at(pickable) != nullptr);

    return *pickables.at(pickable);
}
