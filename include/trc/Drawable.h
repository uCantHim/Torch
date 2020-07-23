#pragma once

#include "base/SceneRegisterable.h"
#include "base/DrawableStatic.h"
#include "Node.h"
#include "PipelineDefinitions.h"

#include "Geometry.h"

namespace trc
{
    using namespace internal;

    //class Geometry;

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

        auto getMaterial() const noexcept -> ui32;
        void setGeometry(Geometry& geo);
        void setMaterial(ui32 matIndex);

        void recordCommandBuffer(Deferred, vk::CommandBuffer cmdBuf);

    protected:
        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;
        ui32 indexCount;

        ui32 material;
    };
}
