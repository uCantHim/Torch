#pragma once

#include <concepts>
#include <stdexcept>
#include <string>
#include <tuple>

#include "trc/Types.h"
#include "trc/assets/AssetBase.h"

namespace trc
{
    /**
     * @brief A dynamic representation of an asset's type
     */
    struct AssetType
    {
        AssetType(const AssetType&) = default;
        AssetType(AssetType&&) noexcept = default;
        AssetType& operator=(const AssetType&) = default;
        AssetType& operator=(AssetType&&) noexcept = default;
        ~AssetType() noexcept = default;

        inline bool operator==(const AssetType& other) const {
            return name == other.name;
        }

        inline bool operator!=(const AssetType& other) const = default;

        inline auto getName() const -> const std::string& {
            return name;
        }

        template<AssetBaseType T>
        inline bool is() const {
            return name == T::name();
        }

        /**
         * @brief Create an AssetType object from an asset type
         */
        template<AssetBaseType T>
        static auto make() -> AssetType {
            return AssetType(T::name());
        }

        /**
         * @brief Create an AssetType object from an asset type's dynamic index
         */
        static auto make(std::string name) -> AssetType {
            return AssetType(std::move(name));
        }

    private:
        AssetType() = delete;
        explicit AssetType(std::string_view _name) : name(_name) {}
        explicit AssetType(std::string _name) : name(std::move(_name)) {}

        std::string name;
    };
} // namespace trc
