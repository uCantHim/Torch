#include "drawable/Drawable.h"

#include "TorchResources.h"
#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"

#include "GBufferPass.h"
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
    Drawable(info, [&]{
        PipelineFeatureFlags flags;
        if (info.transparent) {
            flags |= PipelineFeatureFlagBits::eTransparent;
        }
        if (info.geo.get().hasRig()) {
            flags |= PipelineFeatureFlagBits::eAnimated;
        }

        return getPipeline(flags);
    }())
{
}

Drawable::Drawable(const DrawableCreateInfo& info, Pipeline::ID pipeline)
    :
    deferredPipeline(pipeline),
    animEngine(
        info.geo.get().hasRig()
            ? std::make_unique<AnimationEngine>(*info.geo.get().getRig())
            : std::make_unique<AnimationEngine>()
    ),
    data(std::make_unique<DrawableData>(
        info.geo.get(),
        info.geo,
        info.mat,
        Node::getGlobalTransformID(),
        animEngine->getState()
    )),
    castShadow(info.drawShadow)
{
    deferredSubpass = info.transparent ? GBufferPass::SubPasses::transparency
                                       : GBufferPass::SubPasses::gBuffer;
}

auto Drawable::getMaterial() const -> MaterialID
{
    if (data != nullptr) {
        return data->mat;
    }
    return {};
}

auto Drawable::getGeometry() const -> GeometryID
{
    if (data != nullptr) {
        return data->geoId;
    }
    return {};
}

auto Drawable::getAnimationEngine() noexcept -> AnimationEngine&
{
    return *animEngine;
}

auto Drawable::getAnimationEngine() const noexcept -> const AnimationEngine&
{
    return *animEngine;
}

void Drawable::attachToScene(SceneBase& scene)
{
    if (data == nullptr) return;

    DrawableFunction func;
    if (data->geo.hasRig())
    {
        func = [data=this->data.get()](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
            data->geo.bindVertices(cmdBuf, 0);

            auto layout = *env.currentPipeline->getLayout();
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

            auto layout = *env.currentPipeline->getLayout();
            cmdBuf.pushConstants<mat4>(layout, vk::ShaderStageFlagBits::eVertex, 0,
                                       data->modelMatrixId.get());
            cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eVertex,
                                       sizeof(mat4), static_cast<ui32>(data->mat));
            cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
        };
    }

    deferredRegistration = scene.registerDrawFunction(
        gBufferRenderStage,
        deferredSubpass,
        deferredPipeline,
        std::move(func)
    );
    if (castShadow)
    {
        shadowRegistration = scene.registerDrawFunction(
            shadowRenderStage, SubPass::ID(0),
            getPipeline(PipelineFeatureFlagBits::eShadow),
            [data=this->data.get()](const auto& env, vk::CommandBuffer cmdBuf) {
                drawShadow(*data, env, cmdBuf);
            }
        );
    }
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

    auto layout = *env.currentPipeline->getLayout();
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
