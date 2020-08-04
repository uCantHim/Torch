#pragma once

#include <atomic>

#include "Boilerplate.h"
#include "data_utils/IndexMap.h"

namespace trc
{
    constexpr ui32 NO_PICKABLE = 0;


    class Pickable
    {
    public:
        using ID = ui32;

        Pickable() = default;
        virtual ~Pickable() = default;

        virtual void onPick() = 0;
        virtual void onUnpick() = 0;

        auto getPickableId() const noexcept -> ID;
        void setPickableId(ID pickableId);

    private:
        ID id{ 0 };
    };

    class PickableFunctional : public Pickable
    {
    public:
        PickableFunctional(std::function<void()> onPick,
                           std::function<void()> onUnpick,
                           void* userData = nullptr);

        void onPick() override;
        void onUnpick() override;

    protected:
        std::function<void()> onPickCallback;
        std::function<void()> onUnpickCallback;
        void* userData;
    };

    class PickableRegistry
    {
    public:
        template<typename T, typename ...Args>
        static auto makePickable(Args&&... args) -> T&;
        static void destroyPickable(Pickable& pickable);

        static auto getPickable(Pickable::ID pickable) -> Pickable&;

    private:
        static inline data::IndexMap<Pickable::ID, std::unique_ptr<Pickable>> pickables;
        static inline std::atomic<Pickable::ID> nextId{ 1 };
        static inline std::vector<Pickable::ID> freeIds;
    };



    template<typename T, typename ...Args>
    auto PickableRegistry::makePickable(Args&&... args) -> T&
    {
        static_assert(std::is_base_of_v<Pickable, T>, "Pickable type must inherit from Pickable");

        Pickable::ID id;
        if (!freeIds.empty())
        {
            id = freeIds.back();
            freeIds.pop_back();
        }
        else {
            id = nextId++;
        }
        assert(pickables[id] == nullptr);

        auto& result = *pickables.emplace(id, new T(std::forward<Args>(std::move(args))...));
        result.setPickableId(id);

        return dynamic_cast<T&>(result);
    }
}
