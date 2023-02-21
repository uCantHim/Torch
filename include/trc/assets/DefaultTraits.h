#pragma once

#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetID.h"
#include "trc/assets/AssetManagerInterface.h"
#include "trc/assets/AssetTraits.h"

namespace trc
{
    class ManagerTraits : public AssetTrait
    {
    public:
        virtual void destroy(AssetManagerFwd& manager, AssetID id) = 0;
    };

    template<AssetBaseType T>
    class ManagerTraitsImpl : public ManagerTraits
    {
    public:
        void destroy(AssetManagerFwd& manager, AssetID id) override
        {
            manager.destroy<T>(id);
        }
    };
} // namespace trc
