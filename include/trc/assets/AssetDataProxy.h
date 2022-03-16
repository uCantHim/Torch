#pragma once

#include <variant>

#include "RawData.h"
#include "AssetIds.h"
#include "Material.h"

namespace trc
{
    namespace serial {
        class Asset;
    }

    /**
     * @brief All existing types of intermediate asset data representations
     */
    using AssetDataVariantBase = std::variant<
        GeometryData,
        TextureData,
        MaterialDeviceHandle
    >;

    struct AssetDataProxy
    {
    public:
        AssetDataProxy(const AssetPath& filePath);
        AssetDataProxy(const serial::Asset& asset);
        AssetDataProxy(AssetDataVariantBase var);

        auto getMetaData() const -> const AssetMetaData&;

        template<typename F>
        inline void visit(F&& visitor) const
        {
            std::visit(visitor, variant);
        }

        template<typename T>
        inline bool is() const
        {
            return std::holds_alternative<T>(variant);
        }

        template<typename T>
        inline auto as() const -> const T&
        {
            return std::get<T>(variant);
        }

        void load(const AssetPath& filePath);
        void write(const AssetPath& filePath);

    private:
        void deserialize(const serial::Asset& data);
        auto serialize() const -> serial::Asset;

        AssetMetaData meta;
        AssetDataVariantBase variant;
    };
} // namespace trc
