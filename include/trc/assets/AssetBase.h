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
        typename T::ImportData;
        std::semiregular<T>;
    };

    /**
     * @brief Typedef that retrieves an asset type's registry module
     */
    template<AssetBaseType T>
    using AssetRegistryModule = typename T::Registry;

    /**
     * @brief Typedef that retrieves an asset type's import data type
     */
    template<AssetBaseType T>
    using AssetData = typename T::ImportData;

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
