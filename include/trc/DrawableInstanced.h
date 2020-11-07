#pragma once

#include "GlobalResources.h"
#include "Drawable.h"

namespace trc
{
    using namespace internal;

    /**
     * @brief Purely component-based Drawable class
     */
    class DrawableInstanced : public SceneRegisterable
                            , public Node
                            , public UsePipeline<DrawableInstanced,
                                                 RenderStageTypes::eDeferred,
                                                 DeferredSubPasses::eGBufferPass,
                                                 Pipelines::eDrawableInstancedDeferred>
                            , public UsePipeline<DrawableInstanced,
                                                 RenderStageTypes::eShadow,
                                                 0,
                                                 Pipelines::eDrawableInstancedShadow>
    {
    public:
        using Deferred = PipelineIndex<Pipelines::eDrawableInstancedDeferred>;
        using Shadow = PipelineIndex<Pipelines::eDrawableInstancedShadow>;

        struct InstanceDescription
        {
            InstanceDescription() = default;
            InstanceDescription(const mat4& t, ui32 matIndex)
                : modelMatrix(t), materialIndex(matIndex)
            {}
            InstanceDescription(const Transformation& t, ui32 matIndex)
                : modelMatrix(t.getTransformationMatrix()), materialIndex(matIndex)
            {}

            mat4 modelMatrix;
            ui32 materialIndex;
        };

        DrawableInstanced(ui32 maxInstances, Geometry& geo);
        DrawableInstanced(ui32 maxInstances, Geometry& geo, SceneBase& scene);

        void setGeometry(Geometry& geo);

        void addInstance(InstanceDescription instance);

        void recordCommandBuffer(Deferred, const DrawEnvironment& env, vk::CommandBuffer cmdBuf);
        void recordCommandBuffer(Shadow, const DrawEnvironment& env, vk::CommandBuffer cmdBuf);

    private:
        Geometry* geometry;

        ui32 maxInstances;
        ui32 numInstances{ 0 };
        vkb::Buffer instanceDataBuffer;
    };
} // namespace trc
