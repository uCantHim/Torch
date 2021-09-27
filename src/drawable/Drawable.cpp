#include "drawable/Drawable.h"

#include "TorchResources.h"
#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"
#include "drawable/RasterPipelines.h"

#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"



trc::Drawable::Drawable(GeometryID geo, MaterialID material)
{
    setMaterial(material);
    setGeometry(geo);

    updateDrawFunctions();
}

trc::Drawable::Drawable(GeometryID geo, MaterialID material, SceneBase& scene)
    :
    Drawable(geo, material)
{
    attachToScene(scene);
}

trc::Drawable::Drawable(Drawable&& other) noexcept
    :
    Node(std::forward<Node>(other)),
    currentScene(other.currentScene),
    deferredRegistration(std::move(other.deferredRegistration)),
    shadowRegistration(std::move(other.shadowRegistration)),
    geoIndex(other.geoIndex)
{
    std::swap(drawableDataId, other.drawableDataId);
    std::swap(data, other.data);

    other.removeFromScene();
    updateDrawFunctions();
}

auto trc::Drawable::operator=(Drawable&& rhs) noexcept -> Drawable&
{
    Node::operator=(std::forward<Node>(rhs));

    currentScene = rhs.currentScene;
    deferredRegistration = std::move(rhs.deferredRegistration);
    shadowRegistration = std::move(rhs.shadowRegistration);

    std::swap(drawableDataId, rhs.drawableDataId);
    std::swap(data, rhs.data);

    rhs.removeFromScene();
    updateDrawFunctions();

    return *this;
}

trc::Drawable::~Drawable()
{
    removeFromScene();
    if (data->pickableId != NO_PICKABLE) {
        PickableRegistry::destroyPickable(PickableRegistry::getPickable(data->pickableId));
    }

    DrawableDataStore::free(drawableDataId);
}

auto trc::Drawable::getMaterial() const -> MaterialID
{
    return data->mat;
}

auto trc::Drawable::getGeometry() const -> GeometryID
{
    return geoIndex;
}

void trc::Drawable::setMaterial(MaterialID matIndex)
{
    data->mat = matIndex;
}

void trc::Drawable::setGeometry(GeometryID newGeo)
{
    data->geo = newGeo.get();
    geoIndex = newGeo;
    if (data->geo.hasRig()) {
        data->animEngine = { *data->geo.getRig() };
    }
}

auto trc::Drawable::getAnimationEngine() noexcept -> AnimationEngine&
{
    return data->animEngine;
}

auto trc::Drawable::getAnimationEngine() const noexcept -> const AnimationEngine&
{
    return data->animEngine;
}

void trc::Drawable::enableTransparency()
{
    data->isTransparent = true;
    updateDrawFunctions();
}

void trc::Drawable::attachToScene(SceneBase& scene)
{
    currentScene = &scene;
    updateDrawFunctions();
}

void trc::Drawable::removeFromScene()
{
    currentScene = nullptr;
    deferredRegistration = {};
    shadowRegistration = {};
}

void trc::Drawable::updateDrawFunctions()
{
    if (currentScene == nullptr) return;

    auto bindBaseResources = [data=this->data](const auto& env, vk::CommandBuffer cmdBuf) {
        data->geo.bindVertices(cmdBuf, 0);

        auto layout = env.currentPipeline->getLayout();
        cmdBuf.pushConstants<mat4>(layout, vk::ShaderStageFlagBits::eVertex, 0,
                                   data->modelMatrixId.get());
        cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eVertex,
                                   sizeof(mat4), static_cast<ui32>(data->mat));
    };

    DrawableFunction func;
    Pipeline::ID pipeline;
    if (data->geo.hasRig())
    {
        if (data->pickableId == NO_PICKABLE)
        {
            func = [=, data=this->data](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                bindBaseResources(env, cmdBuf);
                auto layout = env.currentPipeline->getLayout();
                data->animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);

                cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
            };
            pipeline = data->isTransparent ? getDrawableTransparentDeferredAnimatedPipeline()
                                           : getDrawableDeferredAnimatedPipeline();
        }
        else
        {
            func = [=, data=this->data](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                assert(data->pickableId != NO_PICKABLE);

                bindBaseResources(env, cmdBuf);
                auto layout = env.currentPipeline->getLayout();
                data->animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);
                cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84,
                                           data->pickableId);

                cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
            };
            pipeline = data->isTransparent
                ? getDrawableTransparentDeferredAnimatedAndPickablePipeline()
                : getDrawableDeferredAnimatedAndPickablePipeline();
        }
    }
    else
    {
        if (data->pickableId == NO_PICKABLE)
        {
            func = [=, data=this->data](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                bindBaseResources(env, cmdBuf);
                cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
            };
            pipeline = data->isTransparent ? getDrawableTransparentDeferredPipeline()
                                           : getDrawableDeferredPipeline();
        }
        else
        {
            func = [=, data=this->data](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                assert(data->pickableId != NO_PICKABLE);

                bindBaseResources(env, cmdBuf);
                auto layout = env.currentPipeline->getLayout();
                cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84,
                                           data->pickableId);

                cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
            };
            pipeline = data->isTransparent ? getDrawableTransparentDeferredPickablePipeline()
                                           : getDrawableDeferredPickablePipeline();
        }
    }

    deferredRegistration = currentScene->registerDrawFunction(
        RenderStageTypes::getDeferred(),
        data->isTransparent ? RenderPassDeferred::SubPasses::transparency
                            : RenderPassDeferred::SubPasses::gBuffer,
        pipeline,
        std::move(func)
    );
    shadowRegistration = currentScene->registerDrawFunction(
        RenderStageTypes::getShadow(), SubPass::ID(0), getDrawableShadowPipeline(),
        [data=this->data](const auto& env, vk::CommandBuffer cmdBuf) {
            drawShadow(data, env, cmdBuf);
        }
    );
}

void trc::Drawable::drawShadow(
    DrawableData* data,
    const DrawEnvironment& env,
    vk::CommandBuffer cmdBuf)
{
    auto currentRenderPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
    assert(currentRenderPass != nullptr);

    // Set pipeline dynamic states
    uvec2 res = currentRenderPass->getResolution();
    cmdBuf.setViewport(0, vk::Viewport(0.0f, 0.0f, res.x, res.y, 0.0f, 1.0f));
    cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { res.x, res.y }));

    // Bind buffers and push constants
    data->geo.bindVertices(cmdBuf, 0);

    auto layout = env.currentPipeline->getLayout();
    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, data->modelMatrixId.get()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), currentRenderPass->getShadowMatrixIndex()
    );
    data->animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);

    // Draw
    cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
}
