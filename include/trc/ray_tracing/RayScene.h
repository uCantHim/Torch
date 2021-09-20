#pragma once

#include <unordered_map>

#include <vkb/MemoryPool.h>

#include "Transformation.h"
#include "AssetIds.h"
#include "AccelerationStructure.h"
#include "GeometryUtils.h"

namespace trc::rt
{
    struct RayTraceable
    {
        Transformation::ID modelMatrix;

        GeometryID geo;
        MaterialID mat;
    };

    /**
     * @brief
     */
    class RayScene
    {
    public:
        /**
         * @brief
         */
        explicit RayScene(const Instance& instance);

        /**
         *  - Copy model matrices to device
         *  - Update drawable data buffer
         *  - Build TLAS
         */
        void update(vk::CommandBuffer cmdBuf);

        void addDrawable(RayTraceable obj);

        auto getTlas() -> TLAS&;
        auto getTlas() const -> const TLAS&;

        /**
         * Storage buffer containing structs of the format
         *
         *     struct Data
         *     {
         *         ui32 geometryIndex;
         *         ui32 materialIndex;
         *     };
         */
        auto getDrawableDataBuffer() const -> vk::Buffer;

    private:
        static constexpr ui32 MEMORY_POOL_SIZE{ 100000000 };
        static constexpr ui32 MAX_TLAS_INSTANCES{ 2000 };

        const Instance& instance;
        const vkb::Device& device;

        //////////////////////////////////
        // Host-side drawable data storage

        struct DrawableStorage
        {
            RayTraceable info;
            ui32 index;
            BLAS blas;
        };
        std::vector<DrawableStorage> drawables;
        std::vector<Transformation::ID> modelMatrices;


        ////////////////////////////////////////////////////
        // Device-side acceleration structures and instances

        vkb::MemoryPool blasMemoryPool;
        vkb::DeviceMemoryAllocator blasAlloc;

        vkb::Buffer blasInstanceBuffer;
        GeometryInstance* blasInstanceBufferMap;
        TLAS tlas;


        ////////////////////////////////////
        // Device-side drawable shading data

        struct DrawableShadingData
        {
            ui32 geoId;
            ui32 matId;
        };

        vkb::Buffer drawableDataStagingBuffer;
        vkb::Buffer drawableDataDeviceBuffer;
        DrawableShadingData* drawableDataBufferMap;
    };
} // namespace trc::rt
