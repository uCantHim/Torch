#pragma once

#include "Types.h"

namespace trc
{
    struct MaterialDeviceHandle
    {
    public:
        auto getBufferIndex() const -> ui32 {
            return id;
        }

    private:
        friend class MaterialRegistry;

        explicit MaterialDeviceHandle(ui32 materialBufferIndex)
            : id(materialBufferIndex) {}

        ui32 id;
    };
} // namespace trc
