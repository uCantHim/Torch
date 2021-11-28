#include "drawable/Drawable.h"

#include "TorchResources.h"
#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"

#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"



namespace trc
{

Drawable::Drawable(GeometryID geo, MaterialID material)
    :
    Drawable(DrawableCreateInfo{ geo, material })
{
}

Drawable::Drawable(GeometryID geo, MaterialID material, SceneBase& scene)
    :
    Drawable(geo, material)
{
    attachToScene(scene);
}

Drawable::Drawable(const DrawableCreateInfo& info)
    :
    data(std::make_unique<DrawableData>(
        info.geo.get(),
        info.geo,
        info.mat,
        Node::getGlobalTransformID(),
        animEngine.getState()
    ))
{
    PipelineFeatureFlags flags;
    if (info.transparent) {
        flags |= PipelineFeatureFlagBits::eTransparent;
    }
    if (data->geo.hasRig()) {
        flags |= PipelineFeatureFlagBits::eAnimated;
    }

    deferredPipeline = getPipeline(flags);
    deferredSubpass = info.transparent ? RenderPassDeferred::SubPasses::transparency
                                       : RenderPassDeferred::SubPasses::gBuffer;
}

auto Drawable::getMaterial() const -> MaterialID
{
    return data->mat;
}

auto Drawable::getGeometry() const -> GeometryID
{
    return data->geoId;
}

auto Drawable::getAnimationEngine() noexcept -> AnimationEngine&
{
    return animEngine;
}

auto Drawable::getAnimationEngine() const noexcept -> const AnimationEngine&
{
    return animEngine;
}

void Drawable::attachToScene(SceneBase& scene)
{
    DrawableFunction func;
    if (data->geo.hasRig())
    {
        func = [data=this->data.get()](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
            data->geo.bindVertices(cmdBuf, 0);

            auto layout = env.currentPipeline->getLayout();
            cmdBuf.pushConstants<mat4>(layout, vk::ShaderStageFlagBits::eVertex, 0,
                                       data->modelMatrixId.get());
            cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eVertex,
                                       sizeof(mat4), static_cast<ui32>(data->mat));
            cmdBuf.pushConstants<AnimationDeviceData>(
                layout, vk::ShaderStageFlagBits::eVertex, sizeof(mat4) + sizeof(ui32),
                data->anim.get()
            );

            cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
        };
    }
    else
    {
        func = [data=this->data.get()](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
            data->geo.bindVertices(cmdBuf, 0);

            auto layout = env.currentPipeline->getLayout();
            cmdBuf.pushConstants<mat4>(layout, vk::ShaderStageFlagBits::eVertex, 0,
                                       data->modelMatrixId.get());
            cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eVertex,
                                       sizeof(mat4), static_cast<ui32>(data->mat));
            cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
        };
    }

    deferredRegistration = scene.registerDrawFunction(
        deferredRenderStage,
        deferredSubpass,
        deferredPipeline,
        std::move(func)
    );
    shadowRegistration = scene.registerDrawFunction(
        shadowRenderStage, SubPass::ID(0),
        getPipeline(PipelineFeatureFlagBits::eShadow),
        [data=this->data.get()](const auto& env, vk::CommandBuffer cmdBuf) {
            drawShadow(*data, env, cmdBuf);
        }
    );
}

void Drawable::removeFromScene()
{
    deferredRegistration = {};
    shadowRegistration = {};
}

void Drawable::drawShadow(
    const DrawableData& data,
    const DrawEnvironment& env,
    vk::CommandBuffer cmdBuf)
{
    auto currentRenderPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
    assert(currentRenderPass != nullptr);

    // Bind buffers and push constants
    data.geo.bindVertices(cmdBuf, 0);

    auto layout = env.currentPipeline->getLayout();
    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, data.modelMatrixId.get()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), currentRenderPass->getShadowMatrixIndex()
    );
    cmdBuf.pushConstants<AnimationDeviceData>(
        layout, vk::ShaderStageFlagBits::eVertex, sizeof(mat4) + sizeof(ui32),
        data.anim.get()
    );

    // Draw
    cmdBuf.drawIndexed(data.geo.getIndexCount(), 1, 0, 0, 0);
}

} // namespace trc
