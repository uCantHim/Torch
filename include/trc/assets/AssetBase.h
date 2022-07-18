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
        requires std::semiregular<T>;
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
     * @brief Has to be specialized for each asset type
     */
    template<AssetBaseType T>
    class AssetHandle;
} // namespace trc
