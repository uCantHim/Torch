#pragma once

#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetManagerBase.h"
#include "trc/assets/AssetStorage.h"
#include "trc/assets/AssetTraits.h"

namespace trc
{
    class ManagerTraits : public AssetTrait
    {
    public:
        virtual auto create(AssetManagerBase& manager,
                            const AssetPath& path,
                            AssetStorage& storage)
            -> std::optional<AssetID> = 0;

        virtual void destroy(AssetManagerBase& manager, AssetID id) = 0;
    };

    template<AssetBaseType T>
    class ManagerTraitsImpl : public ManagerTraits
    {
    public:
        auto create(AssetManagerBase& manager, const AssetPath& path, AssetStorage& storage)
            -> std::optional<AssetID> override
        {
            if (auto source = storage.loadDeferred<T>(path))
            {
                assert(*source != nullptr);
                return manager.create<T>(std::move(*source));
            }
            return std::nullopt;
        }

        void destroy(AssetManagerBase& manager, AssetID id) override
        {
            manager.destroy<T>(id);
        }
    };
} // namespace trc
