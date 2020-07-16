#pragma once

#include "base/SceneRegisterable.h"
#include "base/DrawableStatic.h"
#include "Node.h"
#include "RenderPassDefinitions.h"

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
        using Lighting = PipelineIndex<Pipelines::eDrawableLighting>;

        Drawable(Geometry& geo, Material& mat);

        auto getGeometry() const noexcept -> const Geometry&;
        auto getMaterial() const noexcept -> const Material&;
        void setGeometry(Geometry& geo);
        void setMaterial(Material& mat);

        void recordCommandBuffer(Deferred, vk::CommandBuffer cmdBuf);
        void recordCommandBuffer(Lighting, vk::CommandBuffer cmdBuf);

    protected:
        Geometry* geometry;
        Material* material;
    };
}
