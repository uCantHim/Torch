#include "drawable/Drawable.h"

#include "TorchResources.h"
#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"
#include "drawable/RasterPipelines.h"

#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"



namespace trc::legacy
{

Drawable::Drawable(GeometryID geo, MaterialID material)
{
    setMaterial(material);
    setGeometry(geo);

    updateDrawFunctions();
}

Drawable::Drawable(GeometryID geo, MaterialID material, SceneBase& scene)
    :
    Drawable(geo, material)
{
    attachToScene(scene);
}

Drawable::Drawable(Drawable&& other) noexcept
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

auto Drawable::operator=(Drawable&& rhs) noexcept -> Drawable&
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

Drawable::~Drawable()
{
    removeFromScene();
    DrawableDataStore::free(drawableDataId);
}

auto Drawable::getMaterial() const -> MaterialID
{
    return data->mat;
}

auto Drawable::getGeometry() const -> GeometryID
{
    return geoIndex;
}

void Drawable::setMaterial(MaterialID matIndex)
{
    data->mat = matIndex;
}

void Drawable::setGeometry(GeometryID newGeo)
{
    data->geo = newGeo.get();
    geoIndex = newGeo;
    if (data->geo.hasRig())
    {
        animEngine = { *data->geo.getRig() };
        data->anim = animEngine.getState();
    }
}

auto Drawable::getAnimationEngine() noexcept -> AnimationEngine&
{
    return animEngine;
}

auto Drawable::getAnimationEngine() const noexcept -> const AnimationEngine&
{
    return animEngine;
}

void Drawable::enableTransparency()
{
    data->isTransparent = true;
    updateDrawFunctions();
}

void Drawable::attachToScene(SceneBase& scene)
{
    currentScene = &scene;
    updateDrawFunctions();
}

void Drawable::removeFromScene()
{
    currentScene = nullptr;
    deferredRegistration = {};
    shadowRegistration = {};
}

void Drawable::updateDrawFunctions()
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
    PipelineFeatureFlags flags;

    if (data->isTransparent) {
        flags |= PipelineFeatureFlagBits::eTransparent;
    }

    if (data->geo.hasRig())
    {
        flags |= PipelineFeatureFlagBits::eAnimated;

        func = [=, data=this->data](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
            bindBaseResources(env, cmdBuf);
            auto layout = env.currentPipeline->getLayout();
            cmdBuf.pushConstants<AnimationDeviceData>(
                layout, vk::ShaderStageFlagBits::eVertex, sizeof(mat4) + sizeof(ui32),
                data->anim.get()
            );

            cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
        };
    }
    else
    {
        func = [=, data=this->data](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
            bindBaseResources(env, cmdBuf);
            cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
        };
    }

    deferredRegistration = currentScene->registerDrawFunction(
        RenderStageTypes::getDeferred(),
        data->isTransparent ? RenderPassDeferred::SubPasses::transparency
                            : RenderPassDeferred::SubPasses::gBuffer,
        getPipeline(flags),
        std::move(func)
    );
    shadowRegistration = currentScene->registerDrawFunction(
        RenderStageTypes::getShadow(), SubPass::ID(0),
        getPipeline(PipelineFeatureFlagBits::eShadow),
        [data=this->data](const auto& env, vk::CommandBuffer cmdBuf) {
            drawShadow(data, env, cmdBuf);
        }
    );
}

void Drawable::drawShadow(
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
    cmdBuf.pushConstants<AnimationDeviceData>(
        layout, vk::ShaderStageFlagBits::eVertex, sizeof(mat4) + sizeof(ui32),
        data->anim.get()
    );

    // Draw
    cmdBuf.drawIndexed(data->geo.getIndexCount(), 1, 0, 0, 0);
}

} // namespace trc::legacy
