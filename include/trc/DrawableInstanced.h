#pragma once

#include "Drawable.h"

namespace trc
{
    using namespace internal;

    /**
     * @brief Purely component-based Drawable class
     */
    class DrawableInstanced : public DrawableBase
                            , public UsePipeline<DrawableInstanced,
                                                 RenderPasses::eDeferredPass,
                                                 DeferredSubPasses::eGBufferPass,
                                                 Pipelines::eDrawableInstancedDeferred>
    {
    public:
        using Deferred = PipelineIndex<Pipelines::eDrawableInstancedDeferred>;

        struct InstanceDescription
        {
            mat4 modelMatrix;
            ui32 materialIndex;
        };

        DrawableInstanced(ui32 maxInstances, Geometry& geo);
        DrawableInstanced(ui32 maxInstances, Geometry& geo, SceneBase& scene);

        void setGeometry(Geometry& geo);

        void addInstance(InstanceDescription instance);

        void recordCommandBuffer(Deferred, vk::CommandBuffer cmdBuf);

    private:
        Geometry* geometry;

        ui32 maxInstances;
        ui32 numInstances{ 0 };
        vkb::Buffer instanceDataBuffer;
    };
} // namespace trc
