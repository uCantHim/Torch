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
                            AssetStorage& storage) -> AssetID = 0;
        virtual void destroy(AssetManagerBase& manager, AssetID id) = 0;
    };

    template<AssetBaseType T>
    class ManagerTraitsImpl : public ManagerTraits
    {
    public:
        auto create(AssetManagerBase& manager, const AssetPath& path, AssetStorage& storage)
            -> AssetID override
        {
            return manager.create<T>(storage.loadDeferred<T>(path));
        }

        void destroy(AssetManagerBase& manager, AssetID id) override
        {
            manager.destroy<T>(id);
        }
    };
} // namespace trc
