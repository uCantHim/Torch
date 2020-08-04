#pragma once

#include <atomic>

#include "Boilerplate.h"
#include "data_utils/IndexMap.h"

namespace trc
{
    constexpr ui32 NO_PICKABLE = 0;

    /**
     * @brief Abstract base class for objects that are pickable by the mouse
     */
    class Pickable
    {
    public:
        using ID = ui32;

        Pickable() = default;
        virtual ~Pickable() = default;

        /**
         * @brief Called when the Pickable is first hovered by the mouse
         */
        virtual void onPick() = 0;

        /**
         * @brief Called when the mouse leaves the Pickable
         */
        virtual void onUnpick() = 0;

        /**
         * @return ID The Pickable's unique ID
         */
        auto getPickableId() const noexcept -> ID;

        /**
         * @brief Set a unique ID for the pickable
         *
         * Do not use this! The program will probably blow up if you mess
         * with this function.
         */
        void setPickableId(ID pickableId);

    private:
        ID id{ 0 };
    };

    /**
     * @brief Pickable with std:function callbacks
     *
     * A convenient dynamic default implementation for a Pickable. Takes
     * pick- and unpick callbacks in the form of std::functions. This
     * approach is much more flexible than creating subclasses for every
     * type of Pickable.
     */
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

    /**
     * @brief Internally used registry for pickables
     *
     * Stores all existing Pickables and assigns unique IDs to new ones.
     */
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
