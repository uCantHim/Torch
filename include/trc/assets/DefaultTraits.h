#pragma once

#include <optional>

#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetManager.h"
#include "trc/assets/AssetTraits.h"

namespace trc
{
    class ManagerTraits : public AssetTrait
    {
    public:
        virtual auto create(AssetManager& manager,
                            const AssetPath& path)
            -> std::optional<AssetID> = 0;

        virtual void destroy(AssetManager& manager, AssetID id) = 0;
    };

    template<AssetBaseType T>
    class ManagerTraitsImpl : public ManagerTraits
    {
    public:
        auto create(AssetManager& manager, const AssetPath& path)
            -> std::optional<AssetID> override
        {
            if (auto id = manager.create<T>(path)) {
                return id->getAssetID();
            }
            return std::nullopt;
        }

        void destroy(AssetManager& manager, AssetID id) override
        {
            manager.destroy<T>(id);
        }
    };
} // namespace trc
