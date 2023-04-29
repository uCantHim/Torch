#pragma once

#include <functional>

#include <componentlib/ComponentStorage.h>
#include <componentlib/ComponentID.h>
#include <trc_util/data/IdPool.h>
#include <trc_util/data/IndexMap.h>

#include "trc/core/RenderStage.h"
#include "trc/core/RenderPass.h"
#include "trc/core/Pipeline.h"
#include "trc/ray_tracing/GeometryUtils.h"

namespace trc
{
    class SceneBase;

    namespace drawcomp {
        struct _DrawableIdTypeTag {};
    }
    using DrawableID = componentlib::ComponentID<drawcomp::_DrawableIdTypeTag>;

    /**
     * @brief
     */
    class DrawableComponentScene
        : public componentlib::ComponentStorage<DrawableComponentScene, DrawableID>
    {
    public:
        struct RayInstanceData
        {
            ui32 geometryIndex;
            ui32 materialIndex;
        };

        DrawableComponentScene(SceneBase& base);

        auto getSceneBase() -> SceneBase&;

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

        /**
         * @brief A more expressive name for `createObject`
         */
        inline auto makeDrawable() -> DrawableID {
            return createObject();
        }

        /**
         * @brief A more expressive name for `deleteObject`
         */
        inline void destroyDrawable(DrawableID drawable) {
            deleteObject(drawable);
        }

    private:
        template<componentlib::ComponentType T>
        friend struct componentlib::ComponentTraits;

        auto allocateRayInstance(RayInstanceData data) -> ui32;
        void freeRayInstance(ui32 index);

        SceneBase* base;

        data::IdPool<ui32> rayInstanceIdPool;

        /**
         * Other than the rt::GeometryInstance data (the format of which is
         * restricted by Vulkan), the custom per-instance data does not have
         * to be tightly packed, but requires constant indices instead.
         */
        data::IndexMap<ui32, RayInstanceData> rayInstances;
    };
} // namespace trc
