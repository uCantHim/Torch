#pragma once

#include "Boilerplate.h"

namespace trc
{
    class Asset
    {
    public:
        using ID = ui32;

        auto getId() const noexcept -> ID {
            return id;
        }

    private:
        template<typename>
        friend class AssetRegistry;

        ID id;
    };
} // namespace trc
