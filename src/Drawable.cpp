#include "Drawable.h"

#include "TorchResources.h"
#include "AssetRegistry.h"
#include "Geometry.h"
#include "Material.h"



trc::Drawable::Drawable(GeometryID geo, MaterialID material)
    :
    Drawable(**AssetRegistry::getGeometry(geo), material)
{
}

trc::Drawable::Drawable(GeometryID geo, MaterialID material, SceneBase& scene)
    :
    Drawable(**AssetRegistry::getGeometry(geo), material, scene)
{
}

trc::Drawable::Drawable(Geometry& geo, MaterialID material, bool transparent)
    :
    geo(&geo),
    isTransparent(transparent)
{
    if (geo.hasRig()) {
        animEngine = { *geo.getRig() };
    }
    setMaterial(material);
    updateDrawFunctions();
}

trc::Drawable::Drawable(Geometry& geo, MaterialID material, SceneBase& scene)
    : Drawable(geo, material)
{
    attachToScene(scene);
}

trc::Drawable::Drawable(Drawable&& other) noexcept
    :
    Node(std::forward<Node>(other)),
    geo(other.geo),
    matIndex(other.matIndex),
    pickableId(other.pickableId),
    isTransparent(other.isTransparent),
    animEngine(std::move(other.animEngine))
{
    if (other.currentScene != nullptr)
    {
        attachToScene(*other.currentScene);
        other.removeFromScene();
    }
    other.geo = nullptr;
    other.matIndex = 0;
    other.pickableId = NO_PICKABLE;
    other.isTransparent = false;
}

auto trc::Drawable::operator=(Drawable&& rhs) noexcept -> Drawable&
{
    Node::operator=(std::forward<Node>(rhs));

    geo = rhs.geo;
    rhs.geo = nullptr;
    matIndex = rhs.matIndex;
    rhs.matIndex = 0;
    pickableId = rhs.pickableId;
    rhs.pickableId = NO_PICKABLE;
    isTransparent = rhs.isTransparent;
    rhs.isTransparent = false;

    animEngine = std::move(rhs.animEngine);

    removeFromScene();
    if (rhs.currentScene != nullptr)
    {
        attachToScene(*rhs.currentScene);
        rhs.removeFromScene();
    }

    return *this;
}

trc::Drawable::~Drawable()
{
    removeFromScene();
    if (pickableId != NO_PICKABLE) {
        PickableRegistry::destroyPickable(PickableRegistry::getPickable(pickableId));
    }
}

void trc::Drawable::setMaterial(MaterialID matIndex)
{
    this->matIndex = matIndex;
}

auto trc::Drawable::getAnimationEngine() noexcept -> AnimationEngine&
{
    return animEngine;
}

auto trc::Drawable::getAnimationEngine() const noexcept -> const AnimationEngine&
{
    return animEngine;
}

void trc::Drawable::enableTransparency()
{
    isTransparent = true;
    if (currentScene != nullptr)
    {
        removeDrawFunctions();
        updateDrawFunctions();
    }
}

void trc::Drawable::attachToScene(SceneBase& scene)
{
    removeFromScene();

    currentScene = &scene;
    updateDrawFunctions();
}

void trc::Drawable::removeFromScene()
{
    if (currentScene != nullptr)
    {
        removeDrawFunctions();
        currentScene = nullptr;
    }
}

void trc::Drawable::removeDrawFunctions()
{
    currentScene->unregisterDrawFunction(deferredRegistration);
    currentScene->unregisterDrawFunction(shadowRegistration);
}

void trc::Drawable::updateDrawFunctions()
{
    if (currentScene == nullptr || geo == nullptr) {
        return;
    }

    DrawableFunction func;
    Pipeline::ID pipeline;
    if (geo->hasRig())
    {

        if (pickableId == NO_PICKABLE) {
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                drawAnimated(env, cmdBuf);
            };
            pipeline = isTransparent ? Pipelines::eDrawableTransparentDeferredAnimated
                                     : Pipelines::eDrawableDeferredAnimated;
        }
        else {
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                drawAnimatedAndPickable(env, cmdBuf);
            };
            pipeline = isTransparent ? Pipelines::eDrawableTransparentDeferredAnimatedAndPickable
                                     : Pipelines::eDrawableDeferredAnimatedAndPickable;
        }
    }
    else
    {
        if (pickableId == NO_PICKABLE) {
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                draw(env, cmdBuf);
            };
            pipeline = isTransparent ? Pipelines::eDrawableTransparentDeferred
                                     : Pipelines::eDrawableDeferred;
        }
        else {
            func = [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
                drawPickable(env, cmdBuf);
            };
            pipeline = isTransparent ? Pipelines::eDrawableTransparentDeferredPickable
                                     : Pipelines::eDrawableDeferredPickable;
        }
    }

    deferredRegistration = currentScene->registerDrawFunction(
        RenderStageTypes::eDeferred,
        isTransparent ? DeferredSubPasses::eTransparencyPass : DeferredSubPasses::eGBufferPass,
        pipeline,
        std::move(func)
    );
    shadowRegistration = currentScene->registerDrawFunction(
        RenderStageTypes::eShadow, 0, Pipelines::eDrawableShadow,
        [this](const auto& env, vk::CommandBuffer cmdBuf) { drawShadow(env, cmdBuf); }
    );
}

void trc::Drawable::prepareDraw(vk::CommandBuffer cmdBuf, vk::PipelineLayout layout)
{
    cmdBuf.bindIndexBuffer(geo->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, geo->getVertexBuffer(), vk::DeviceSize(0));

    cmdBuf.pushConstants<mat4>(layout, vk::ShaderStageFlagBits::eVertex, 0, getGlobalTransform());
    cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eVertex,
                               sizeof(mat4), static_cast<ui32>(matIndex));
}

void trc::Drawable::draw(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawAnimated(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawPickable(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    assert(pickableId != NO_PICKABLE);

    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);
    cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84, pickableId);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawAnimatedAndPickable(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    assert(pickableId != NO_PICKABLE);

    auto layout = env.currentPipeline->getLayout();
    prepareDraw(cmdBuf, layout);
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);
    cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eFragment, 84, pickableId);

    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}

void trc::Drawable::drawShadow(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    auto currentRenderPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
    assert(currentRenderPass != nullptr);

    // Set pipeline dynamic states
    uvec2 res = currentRenderPass->getResolution();
    cmdBuf.setViewport(0, vk::Viewport(0.0f, 0.0f, res.x, res.y, 0.0f, 1.0f));
    cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { res.x, res.y }));

    // Bind buffers and push constants
    cmdBuf.bindIndexBuffer(geo->getIndexBuffer(), 0, vk::IndexType::eUint32);
    cmdBuf.bindVertexBuffers(0, geo->getVertexBuffer(), vk::DeviceSize(0));

    auto layout = env.currentPipeline->getLayout();
    cmdBuf.pushConstants<mat4>(
        layout, vk::ShaderStageFlagBits::eVertex,
        0, getGlobalTransform()
    );
    cmdBuf.pushConstants<ui32>(
        layout, vk::ShaderStageFlagBits::eVertex,
        sizeof(mat4), currentRenderPass->getShadowMatrixIndex()
    );
    animEngine.pushConstants(sizeof(mat4) + sizeof(ui32), layout, cmdBuf);

    // Draw
    cmdBuf.drawIndexed(geo->getIndexCount(), 1, 0, 0, 0);
}
