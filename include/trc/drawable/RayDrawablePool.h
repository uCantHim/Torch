#pragma once

#include <vector>
#include <mutex>
#include <future>

#include <vkb/MemoryPool.h>

#include "DrawablePoolStructs.h"

#include "ray_tracing/AccelerationStructure.h"
#include "ray_tracing/GeometryUtils.h"

namespace trc
{
    struct RayDrawablePoolCreateInfo
    {
        ui32 maxInstances{ 1000 };
    };

    /**
     * @brief
     */
    class RayDrawablePool
    {
    public:
        /**
         * @brief
         */
        explicit RayDrawablePool(const ::trc::Instance& instance,
                                 const RayDrawablePoolCreateInfo& info = {});

        /**
         * @brief Rebuild the TLAS
         *
         * Create a new command buffer from the device and use it to build
         * the TLAS asynchronously.
         */
        auto buildTlas() -> std::future<void>;

        /**
         * @brief Rebuild the TLAS
         *
         * Collect instances, update model matrices, update the drawable
         * data lookup table, and build the TLAS.
         *
         * @param vk::CommandBuffer cmdBuf Must have compute capability.
         */
        void buildTlas(vk::CommandBuffer cmdBuf);

        void createDrawable(ui32 drawableId, const DrawableCreateInfo& info);
        void deleteDrawable(ui32 drawableId);
        void createInstance(ui32 drawableId, const DrawableInstanceCreateInfo& info);
        void deleteInstance(ui32 drawableId, ui32 instanceId);

        auto getTlas() -> rt::TLAS&;
        auto getTlas() const -> const rt::TLAS&;

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
        const ::trc::Instance& instance;

        struct Instance
        {
            Transformation::ID transform;
            AnimationEngine::ID animData;
        };

        struct DrawableData
        {
            u_ptr<rt::BLAS> blas;

            std::vector<Instance> instances;
        };

        std::vector<DrawableData> drawables;
        i32 highestDrawableIndex{ 0 };
        std::mutex allInstancesLock;

        struct DrawableShadingData
        {
            ui32 geoId;
            ui32 matId;
        };

        vkb::Buffer drawableDataStagingBuffer;
        vkb::Buffer drawableDataDeviceBuffer;
        DrawableShadingData* drawableDataBufferMap;

        // TLAS stuff
        rt::TLAS tlas;

        vkb::Buffer tlasBuildDataBuffer;  // rt::GeometryInstance[]
        rt::GeometryInstance* tlasBuildDataBufferMap;

        // BLAS memory allocation
        vkb::MemoryPool blasMemoryPool;
        vkb::DeviceMemoryAllocator blasAlloc;
    };
} // namespace trc
