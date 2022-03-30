#pragma once

#include <concepts>

namespace trc
{
    /**
     * @brief Static interface requirements for an asset type
     */
    template<typename T>
    concept AssetBaseType = requires {
        typename T::Registry;
        std::semiregular<T>;
    };

    /**
     * @brief Typedef that retrieves an asset type's registry module
     */
    template<AssetBaseType T>
    using AssetRegistryModule = typename T::Registry;

    /**
     * @brief Forward declaration that has to be specialized for each asset type
     */
    template<AssetBaseType T>
    struct AssetData;

    /**
     * @brief Typedef that retrieves an asset type's handle type
     */
    template<AssetBaseType T>
        requires requires {
            typename AssetRegistryModule<T>::Handle;
            std::semiregular<typename AssetRegistryModule<T>::Handle>;
        }
    using AssetHandle = typename AssetRegistryModule<T>::Handle;
} // namespace trc
