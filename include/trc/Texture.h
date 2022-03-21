#pragma once

#include "Types.h"

namespace trc
{
    /**
     * @brief Handle to a texture stored in the asset registry
     *
     * TODO: Remove getDefaultSampler function from vkb::Image and manage
     * samplers through this class?
     */
    class TextureDeviceHandle
    {
    public:
        auto getDeviceIndex() const -> ui32 {
            return deviceIndex;
        }

    private:
        friend class TextureRegistry;
        explicit TextureDeviceHandle(ui32 index) : deviceIndex(index) {}

        ui32 deviceIndex;
    };
} // namespace trc
