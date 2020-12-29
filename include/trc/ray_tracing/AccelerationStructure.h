#pragma once

#include "GeometryUtils.h"

namespace trc::rt
{
    namespace internal
    {
        class AccelerationStructureBase
        {
        protected:
            AccelerationStructureBase() = default;

        public:
            /**
             * @return The underlying acceleration structure handle
             */
            auto operator*() const noexcept -> vk::AccelerationStructureKHR;

        protected:
            using UniqueAccelerationStructure = vk::UniqueHandle<
                vk::AccelerationStructureKHR, vk::DispatchLoaderDynamic
            >;

            /**
             * @brief Create or recreate the acceleration structure
             */
            void create(vk::AccelerationStructureBuildGeometryInfoKHR buildInfo,
                        const vk::ArrayProxy<const ui32>& primitiveCount);

            vk::AccelerationStructureBuildGeometryInfoKHR geoBuildInfo;

            vk::AccelerationStructureBuildSizesInfoKHR buildSizes;
            vkb::DeviceLocalBuffer accelerationStructureBuffer;
            UniqueAccelerationStructure accelerationStructure;
        };
    }

    /**
     * @brief A bottom-level acceleration structure
     */
    class BottomLevelAccelerationStructure : public internal::AccelerationStructureBase
    {
    public:
        /**
         * Does not build the acceleration structure. This gives you the
         * opportunity to optimize by building multiple acceleration
         * structures at once.
         */
        explicit BottomLevelAccelerationStructure(GeometryID geo);

        /**
         * Does not build the acceleration structure. This gives you the
         * opportunity to optimize by building multiple acceleration
         * structures at once.
         */
        explicit BottomLevelAccelerationStructure(std::vector<GeometryID> geos);

        /**
         * @brief Build the acceleration structure
         *
         * It is advised to use vku::buildAccelerationStructures() to build
         * multiple acceleration structures at once.
         */
        void build();

        auto getDeviceAddress() const noexcept -> ui64;

    private:
        std::vector<vk::AccelerationStructureGeometryKHR> geometries;
        std::vector<ui32> primitiveCounts;
        ui64 deviceAddress;
    };


    /**
     * @brief A collection of geometry instances that can be raytraced
     */
    class TopLevelAccelerationStructure : public internal::AccelerationStructureBase
    {
    public:
        TopLevelAccelerationStructure();

        /**
         * @brief Create a top level acceleration structure
         *
         * One must specify a maximum number of contained geometry instances at
         * creation time because the number of drawn instances may vary over
         * time.
         *
         * I believe it is best to think about a TLAS like a scene. According to
         * an Nvidia article it's best to just rebuild the TLAS every time its
         * instances change (move, get destroyed, get created, etc.).
         *
         * @param uint32_t maxInstances The maximum number of geometry
         *                              instances in the TLAS
         */
        explicit TopLevelAccelerationStructure(uint32_t maxInstances);

        /**
         * @brief Build the TLAS from a buffer of instances
         *
         * Remaining instances are discarded if the number of instances in the
         * vector is greater than maxInstances.
         */
        void build(const vkb::Buffer& instanceBuffer, ui32 offset = 0);

    private:
        uint32_t maxInstances;
        vk::AccelerationStructureGeometryKHR geometry;

        /**
         * The top level AS keeps the scratch buffer because it is updated
         * way more often than the bottom level AS
         */
        vkb::Buffer scratchBuffer;
        vkb::Buffer instanceBuffer;
    };


    // ---------------- //
    //      Helpers     //
    // ---------------- //

    /**
     * @brief Build multiple acceleration structures at once
     */
    void buildAccelerationStructures(const std::vector<BottomLevelAccelerationStructure>& as);

    using BLAS = BottomLevelAccelerationStructure;
    using TLAS = TopLevelAccelerationStructure;
} // namespace trc::rt
