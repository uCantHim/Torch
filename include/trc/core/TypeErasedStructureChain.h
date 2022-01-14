#pragma once

#include <concepts>
#include <tuple>

#include "../Types.h"

namespace trc
{
    template<typename T, typename ...Us>
    constexpr bool first_is = std::same_as<std::tuple_element_t<0, std::tuple<Us...>>, T>;

    class TypeErasedStructureChain
    {
    public:
        TypeErasedStructureChain();

        /**
         * Ensure that vk::PhysicalDeviceFeatures2 is not included because
         * we need a reliable format here. Otherwise we could include the
         * same structure twice in the chain, which is not allowed.
         */
        template<typename ...Ts>
            requires (!first_is<vk::PhysicalDeviceFeatures2, Ts...>)
        TypeErasedStructureChain(Ts&&... ts);

        ~TypeErasedStructureChain();

        /**
         * @return void* Pointer to the first element in the chain. Set
         *               this as the feature chain's pNext.
         */
        auto getPNext() const -> void*;

    private:
        void* data;
        void(*destroy)(void*);
        void*(*getFirstStructure)(void*);
    };


    template<typename ...Ts>
        requires (!first_is<vk::PhysicalDeviceFeatures2, Ts...>)
    TypeErasedStructureChain::TypeErasedStructureChain(Ts&&... ts)
        :
        data(new vk::StructureChain{ std::forward<Ts>(ts)... }),
        destroy([](void* data) {
            delete static_cast<vk::StructureChain<Ts...>*>(data);
        }),
        getFirstStructure([](void* data) -> void* {
            // Skip vk::PhysicalDeviceFeatures2 to avoid having
            // duplicates in the pNext chain
            return &static_cast<vk::StructureChain<Ts...>*>(data)->get();
        })
    {}
} // namespace trc
