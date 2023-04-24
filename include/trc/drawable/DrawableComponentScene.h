#pragma once

#include <functional>

#include <componentlib/ComponentStorage.h>
#include <componentlib/ComponentID.h>

#include "trc/Transformation.h"

#include "trc/assets/Geometry.h"

#include "trc/core/RenderStage.h"
#include "trc/core/RenderPass.h"
#include "trc/core/Pipeline.h"
#include "trc/core/SceneBase.h"

#include "trc/drawable/NodeComponent.h"
#include "trc/drawable/AnimationComponent.h"
#include "trc/drawable/RasterizationComponent.h"

#include "trc/ray_tracing/GeometryUtils.h"

namespace trc
{
    class SceneBase;

    struct RasterComponentCreateInfo
    {
        struct DrawFunctionSpec
        {
            RenderStage::ID stage;
            SubPass::ID subPass;
            Pipeline::ID pipeline;
            std::function<
                void(drawcomp::RasterComponent&, const DrawEnvironment&, vk::CommandBuffer)
            > func;
        };

        drawcomp::RasterComponent drawData;
        std::vector<DrawFunctionSpec> drawFunctions;
    };

    struct RayComponentCreateInfo
    {
        GeometryID geo;
        MaterialID mat;

        Transformation::ID transformation;
    };

    namespace drawcomp {
        struct _DrawableIdTypeTag {};
    }
    using DrawableID = componentlib::ComponentID<drawcomp::_DrawableIdTypeTag>;

    class DrawableComponentScene;

    /**
     * @brief A move-only unique wrapper for DrawableID
     *
     * Calls scene.destroyDrawable(this) on destruction.
     */
    struct UniqueDrawableID
    {
    public:
        UniqueDrawableID(const UniqueDrawableID&) = delete;
        auto operator=(const UniqueDrawableID&) noexcept -> UniqueDrawableID& = delete;

        UniqueDrawableID() = default;
        UniqueDrawableID(DrawableComponentScene& scene, DrawableID id);
        UniqueDrawableID(UniqueDrawableID&& other) noexcept;
        auto operator=(UniqueDrawableID&& other) noexcept -> UniqueDrawableID&;
        ~UniqueDrawableID() noexcept;

        operator bool() const;
        operator DrawableID() const;
        auto operator*() const -> DrawableID;
        auto get() const -> DrawableID;

    private:
        DrawableComponentScene* scene{ nullptr };
        DrawableID id;
    };

    /**
     * @brief
     */
    class DrawableComponentScene
    {
    public:
        struct RayInstanceData
        {
            ui32 geometryIndex;
            ui32 materialIndex;
        };

        DrawableComponentScene(SceneBase& base);

        void updateAnimations(float timeDelta);
        void updateRayData();

        /**
         * @brief Get the maximum size of ray-instance device data
         *
         * @return size_t The minimum required size for buffers passed to
         *                `DrawableComponentScene::writeRayDeviceData` to hold
         *                all ray-instance data.
         */
        auto getMaxRayDeviceDataSize() const -> size_t;

        /**
         * @return ui32 The current number of ray-traced objects. Any buffer
         *              passed to `DrawableComponentScene::writeTlasInstances`
         *              should be at least big enough to hold this many
         *              `rt::GeometryInstance` objects.
         */
        auto getMaxRayGeometryInstances() const -> ui32;

        /**
         * @param rt::GeometryInstance* instanceBuf The memory to which to write
         *        the geometry instance data.
         * @param ui32 maxInstances Restricts the maximum number of instances
         *        written to the buffer. Used as a safeguard against possible
         *        out-of-bounds writes. The caller *should* size `instanceBuf`
         *        appropriately with information from
         *        `DrawableComponentScene::getMaxRayGeometryInstances`.
         *
         * @return ui32 Number of instances written to the buffer
         */
        auto writeTlasInstances(rt::GeometryInstance* instanceBuf, ui32 maxInstances) const
            -> ui32;

        /**
         * @param void* deviceDataBuf
         * @param size_t maxSize Restricts the maximum number of bytes written
         *        to the buffer. Used as a safeguard against possible out-of-
         *        bounds writes. The caller *should* size `instanceBuf`
         *        appropriately with information from
         *        `DrawableComponentScene::getMaxRayDeviceDataSize`.
         *
         * @return size_t Number of bytes actually written to `deviceDataBuf`.
         */
        auto writeRayDeviceData(void* deviceDataBuf, size_t maxSize) const
            -> size_t;

        auto makeDrawable() -> DrawableID;
        auto makeUniqueDrawable() -> UniqueDrawableID;
        void destroyDrawable(DrawableID drawable);

        void makeRasterization(DrawableID drawable, const RasterComponentCreateInfo& createInfo);
        void makeRaytracing(DrawableID drawable, const RayComponentCreateInfo& createInfo);
        auto makeAnimationEngine(DrawableID drawable, RigHandle rig) -> AnimationEngine&;
        auto makeNode(DrawableID drawable) -> Node&;

        bool hasRasterization(DrawableID drawable) const;
        bool hasRaytracing(DrawableID drawable) const;
        bool hasAnimation(DrawableID drawable) const;
        bool hasNode(DrawableID drawable) const;

        auto getRasterization(DrawableID drawable) -> const drawcomp::RasterComponent&;
        auto getAnimationEngine(DrawableID drawable) -> AnimationEngine&;
        auto getNode(DrawableID drawable) -> Node&;

    private:
        template<componentlib::ComponentType T>
        friend struct componentlib::ComponentTraits;

        struct RayComponent
        {
            Transformation::ID modelMatrix;
            GeometryHandle geo;  // Keep the geometry alive
            ui32 materialIndex;

            ui32 instanceDataIndex;
        };

        struct InternalStorage : componentlib::ComponentStorage<InternalStorage, DrawableID>
        {
            auto allocateRayInstance(RayInstanceData data) -> ui32;
            void freeRayInstance(ui32 index);

            data::IdPool<ui32> rayInstanceIdPool;

            /**
             * Other than the rt::GeometryInstance data (the format of which is
             * restricted by Vulkan), the custom per-instance data does not have
             * to be tightly packed, but requires constant indices instead.
             */
            data::IndexMap<ui32, RayInstanceData> rayInstances;
        };

        SceneBase* base;
        InternalStorage storage;
    };
} // namespace trc

template<>
struct componentlib::ComponentTraits<trc::DrawableComponentScene::RayComponent>
{
    void onCreate(trc::DrawableComponentScene::InternalStorage& storage,
                  trc::DrawableID drawable,
                  trc::DrawableComponentScene::RayComponent& ray)
    {
        // Allocate a user data structure
        //
        // This data is referenced by geometry instances via the instanceCustomIndex
        // property and defines auxiliary information that Torch needs to draw
        // ray traced objects.
        ray.instanceDataIndex = storage.allocateRayInstance(
            trc::DrawableComponentScene::RayInstanceData{
                .geometryIndex=ray.geo.getDeviceIndex(),
                .materialIndex=ray.materialIndex,
            }
        );

        // Allocate a geometry instance
        //
        // This data communicates definitions of ray traced object to Vulkan.
        storage.add<trc::rt::GeometryInstance>(drawable,
            trc::rt::GeometryInstance(
                trc::mat4{ 1.0f },
                ray.instanceDataIndex,
                0xff, 0,
                vk::GeometryInstanceFlagBitsKHR::eForceOpaque
                | vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable,
                ray.geo.getAccelerationStructure()
            )
        );
    }

    void onDelete(trc::DrawableComponentScene::InternalStorage& storage,
                  auto /*id*/,
                  trc::DrawableComponentScene::RayComponent ray)
    {
        storage.freeRayInstance(ray.instanceDataIndex);
    }
};
