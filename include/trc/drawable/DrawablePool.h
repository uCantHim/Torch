#pragma once

#include <vector>
#include <mutex>

#include "Types.h"
#include "AnimationEngine.h"
#include "DrawablePoolStructs.h"
#include "RasterDrawablePool.h"
#include "RayDrawablePool.h"

namespace trc
{
    class SceneBase;

    struct DrawablePoolCreateInfo
    {
        /**
         * Has a slightly different meaning for rasterized and ray traced
         * objects:
         *
         * Rasterized: Maximum number of drawables, but each drawable may
         *             have an unlimited number of instances.
         * Ray Traced: Maximum total number of instances.
         */
        ui32 maxInstances{ 1000 };

        /** Initialize ray tracing if it is additionally enabled on the instance */
        bool initRayTracing{ true };
    };

    /**
     * @brief Scene class for shared rasterized/ray traced drawables
     *
     * Ray tracing is enabled if it is enabled on the trc::Instance passed
     * to the constructor.
     */
    class DrawablePool
    {
    public:
        class InstanceHandle;
        using Handle = InstanceHandle*;

        /**
         * @brief Handle to an instance of a drawble object
         *
         * Must call InstanceHandle::destroy to delete the instance from
         * its owning DrawablePool.
         */
        class InstanceHandle : public Node
        {
        public:
            InstanceHandle(const InstanceHandle&) = delete;
            InstanceHandle(InstanceHandle&&) noexcept = delete;
            auto operator=(const InstanceHandle&) -> InstanceHandle& = delete;
            auto operator=(InstanceHandle&&) noexcept -> InstanceHandle& = delete;

            /**
             * @brief Create another instance of this drawable
             */
            auto copy() -> Handle;

            /**
             * @brief Remove this instance from its DrawablePool
             */
            void destroy();

            auto getGeometry() const -> GeometryID;
            auto getMaterial() const -> MaterialID;

            auto getAnimationEngine() -> AnimationEngine&;
            auto getAnimationEngine() const -> const AnimationEngine&;

        private:
            friend class DrawablePool;
            friend u_ptr<InstanceHandle>::deleter_type;
            InstanceHandle(DrawablePool* pool, ui32 drawDataId, ui32 instanceId);
            ~InstanceHandle() = default;

            // Inherits from Node, so that one is safe
            AnimationEngine animEngine;

            DrawablePool* pool;
            ui32 drawableId;
            ui32 instanceId;
        };

        /**
         * @brief Create a pool for rasterized and/or ray traced drawables
         */
        DrawablePool(const ::trc::Instance& instance, const DrawablePoolCreateInfo& info = {});

        /**
         * @brief Create a pool for rasterized and/or ray traced drawables
         */
        DrawablePool(const ::trc::Instance& instance,
                     const DrawablePoolCreateInfo& info,
                     SceneBase& scene);

        ~DrawablePool() = default;

        DrawablePool(const DrawablePool&) = delete;
        DrawablePool(DrawablePool&&) noexcept = delete;
        auto operator=(const DrawablePool&) -> DrawablePool& = delete;
        auto operator=(DrawablePool&&) noexcept -> DrawablePool& = delete;

        void attachToScene(SceneBase& scene);

        /**
         * @brief Create a drawable
         *
         * Automatically creates one instance of the drawable.
         *
         * It is possible to create multiple instances of the same drawable
         * through the returned handle.
         *
         * @return Handle A handle to the drawable's first instance
         */
        auto create(const DrawableCreateInfo& info) -> Handle;

        /**
         * @brief Destroy an instance
         *
         * Deletes only the specified instance, but not all instances of
         * the underlying drawable data. Deletes the entire drawable if
         * the destroyed instance is the last one for its parent drawable.
         *
         * @param Handle instance The handle is not usable anymore after
         *        this call. Access will result in segfaults.
         */
        void destroy(Handle instance);

        /**
         * Build TLAS (if ray tracing is enabled)
         */
        void update();

        /**
         * @throw std::runtime_error if ray tracing is not enabled
         */
        auto getRayResources() const -> std::pair<vk::AccelerationStructureKHR, vk::Buffer>;

    private:
        const Instance& instance;

        //////////////////////////
        // Drawables and Instances

        struct Drawable
        {
            std::vector<u_ptr<InstanceHandle>> instances;

            GeometryID geoId;
            MaterialID matId;
            Geometry geo;

            bool isRasterized;
            bool isRayTraced;
        };

        auto createDrawable(const DrawableCreateInfo& info) -> Handle;
        void deleteDrawable(ui32 drawableId);
        auto createInstance(ui32 drawableId) -> Handle;
        void deleteInstance(Handle instance);

        data::IdPool drawableIdPool;
        std::vector<Drawable> drawables;


        ////////////////
        // Rasterization

        RasterDrawablePool raster;


        //////////////
        // Ray Tracing

        u_ptr<RayDrawablePool> ray;
    };
} // namespace trc
