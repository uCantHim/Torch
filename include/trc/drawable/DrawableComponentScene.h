#pragma once

#include <functional>

#include <componentlib/ComponentStorage.h>
#include <componentlib/ComponentID.h>

#include "core/RenderStage.h"
#include "core/RenderPass.h"
#include "core/Pipeline.h"
#include "core/SceneBase.h"

#include "drawable/NodeComponent.h"
#include "drawable/AnimationComponent.h"
#include "drawable/RasterizationComponent.h"
#include "drawable/RaytracingComponent.h"

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
        DrawableComponentScene(SceneBase& base);

        void updateAnimations(float timeDelta);

        auto makeDrawable() -> DrawableID;
        auto makeDrawableUnique() -> UniqueDrawableID;
        void destroyDrawable(DrawableID drawable);

        void makeRasterization(DrawableID drawable, const RasterComponentCreateInfo& createInfo);
        void makeRaytracing(DrawableID drawable);
        void makeAnimation(DrawableID drawable, Rig& rig);
        void makeNode(DrawableID drawable);

        bool hasRasterization(DrawableID drawable) const;
        bool hasRaytracing(DrawableID drawable) const;
        bool hasAnimation(DrawableID drawable) const;
        bool hasNode(DrawableID drawable) const;

        auto getRasterization(DrawableID drawable) -> const drawcomp::RasterComponent&;
        auto getRaytracing(DrawableID drawable) -> const drawcomp::RayComponent&;
        auto getAnimationEngine(DrawableID drawable) -> AnimationEngine&;
        auto getNode(DrawableID drawable) -> Node&;

    private:
        struct InternalStorage : componentlib::ComponentStorage<InternalStorage, DrawableID> {};

        SceneBase* base;
        InternalStorage storage;
    };
} // namespace trc
