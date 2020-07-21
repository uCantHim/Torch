#pragma once

#include "base/SceneRegisterable.h"
#include "base/DrawableStatic.h"
#include "Node.h"
#include "PipelineDefinitions.h"

namespace trc
{
    using namespace internal;

    class Geometry;
    class Material;

    class DrawableBase : public SceneRegisterable, public Node {};

    /**
     * @brief Purely component-based Drawable class
     */
    class Drawable : public DrawableBase
                   , public UsePipeline<Drawable,
                                        RenderPasses::eDeferredPass,
                                        Pipelines::eDrawableDeferred>
    {
    public:
        using Deferred = PipelineIndex<Pipelines::eDrawableDeferred>;

        Drawable(Geometry& geo, ui32 mat);
        Drawable(Geometry& geo, ui32 mat, SceneBase& scene);

        auto getGeometry() const noexcept -> const Geometry&;
        auto getMaterial() const noexcept -> ui32;
        void setGeometry(Geometry& geo);
        void setMaterial(ui32 matIndex);

        void recordCommandBuffer(Deferred, vk::CommandBuffer cmdBuf);

    protected:
        Geometry* geometry;
        ui32 material;
    };
}
