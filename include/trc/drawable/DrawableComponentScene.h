#pragma once

#include <componentlib/ComponentStorage.h>
#include <componentlib/ComponentID.h>
#include <trc_util/data/IdPool.h>
#include <trc_util/data/IndexMap.h>

#include "trc/RasterSceneModule.h"
#include "trc/RaySceneModule.h"

namespace trc
{
    class RasterSceneBase;
    class DrawableComponentScene;

    namespace drawcomp {
        struct _DrawableIdTypeTag {};
    }
    using DrawableID = componentlib::ComponentID<drawcomp::_DrawableIdTypeTag>;

    struct UniqueDrawableID
    {
        UniqueDrawableID(const UniqueDrawableID&) = delete;
        UniqueDrawableID& operator=(const UniqueDrawableID&) = delete;

        UniqueDrawableID(UniqueDrawableID&&) noexcept;
        UniqueDrawableID& operator=(UniqueDrawableID&&) noexcept;
        ~UniqueDrawableID() noexcept;

        UniqueDrawableID() = default;
        UniqueDrawableID(DrawableID drawable, DrawableComponentScene& scene);

        constexpr auto operator*() const -> DrawableID {
            return id;
        }

    private:
        DrawableID id{ DrawableID::NONE };
        DrawableComponentScene* scene{ nullptr };
    };

    /**
     * @brief
     */
    class DrawableComponentScene
        : public componentlib::ComponentStorage<DrawableComponentScene, DrawableID>
        , public RaySceneModule
    {
    public:
        DrawableComponentScene(RasterSceneBase& base);

        auto getSceneBase() -> RasterSceneBase&;

        void updateAnimations(float timeDelta);

        /**
         * @brief Update transformations of ray geometry instances
         */
        void updateRayInstances();

        /**
         * @brief A more expressive name for `createObject`
         */
        inline auto makeDrawable() -> DrawableID {
            return createObject();
        }

        inline auto makeDrawableUnique() -> UniqueDrawableID {
            return UniqueDrawableID{ makeDrawable(), *this };
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

        RasterSceneBase* base;
    };
} // namespace trc
