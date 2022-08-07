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
     *
     * Two methods *must* be implemented for an AssetData<> specialization:
     *
     *  - AssetData<T>::serialize    of the type `void(std::ostream&)`
     *  - AssetData<T>::deserialize  of the type `void(std::istream&)`
     *
     * A method `AssetData<T>::resolveReferences(AssetManager&)` may be
     * implemented if the data contains references to other assets. In this
     * method, `AssetReference<>::resolve` should be called for all
     * references associated with the AssetData object.
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
        requires requires (AssetData<T> data, std::istream& is) {
            { data.deserialize(is) } -> std::same_as<void>;
        };
        requires requires (const AssetData<T> data, std::ostream& os) {
            { data.serialize(os) } -> std::same_as<void>;
        };
    };
} // namespace trc
