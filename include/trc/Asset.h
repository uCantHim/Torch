#pragma once

#include "Boilerplate.h"

namespace trc
{
    class Asset
    {
    public:
        inline auto getAssetId() const noexcept -> ui32 {
            return id;
        }

    private:
        friend class AssetRegistry;
        ui32 id{ UINT32_MAX };
    };
} // namespace trc
