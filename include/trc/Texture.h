#pragma once

namespace trc
{
    /**
     * @brief Handle to a texture stored in the asset registry
     *
     * Abstraction over vkb::Image, which should just act as the basic
     * memory-managing storage unit.
     *
     * TODO: Remove getDefaultSampler function from vkb::Image and manage
     * samplers through this class?
     */
    class TextureDeviceHandle
    {
    public:
        /**
         * @brief
         */
        TextureDeviceHandle() = default;

    private:

    };
} // namespace trc
