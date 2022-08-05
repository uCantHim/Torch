#pragma once

#include <concepts>
#include <iostream>

namespace trc
{
    template<typename T>
    concept IsCompleteType = requires {
        sizeof(T);
    };

    /**
     * @brief Typedef that retrieves an asset type's registry module
     */
    template<typename T>
    using AssetRegistryModule = typename T::Registry;

    /**
     * @brief Forward declaration that has to be specialized for each asset type
     */
    template<typename T>
    struct AssetData;

    /**
     * @brief Has to be specialized for each asset type
     */
    template<typename T>
    class AssetHandle;

    /**
     * @brief Static interface requirements for an asset type
     */
    template<typename T>
    concept AssetBaseType = requires {
        typename T::Registry;
        requires std::semiregular<T>;
        requires IsCompleteType<AssetData<T>>;
        // requires requires (std::ostream& os) {
        //     AssetData<T>::serialize(os);
        //     AssetData<T>::deserialize(os) -> std::template same_as<AssetData<T>>;
        // }
        // requires IsCompleteType<AssetHandle<T>>;
    };
} // namespace trc
