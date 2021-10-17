#include "drawable/RasterDrawablePool.h"

#include "util/algorithm/VectorTransform.h"
#include "TorchResources.h"
#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"
#include "PipelineDefinitions.h"
#include "drawable/RasterPipelines.h"



trc::RasterDrawablePool::RasterDrawablePool(const RasterDrawablePoolCreateInfo& info)
    :
    drawables(info.maxDrawables),
    drawableMetas(info.maxDrawables)
{
    createRasterFunctions();
}

void trc::RasterDrawablePool::attachToScene(SceneBase& scene)
{
    for (auto& [pipeline, funcDef] : drawFunctions)
    {
        auto& [func, subpass, renderpass] = funcDef;
        drawRegistrations.emplace_back(
            scene.registerDrawFunction(renderpass, subpass, pipeline, func)
        );
    }
}

void trc::RasterDrawablePool::createDrawable(ui32 drawableId, const DrawableCreateInfo& info)
{
    DrawableData& d = drawables.at(drawableId);
    assert(d.instances.empty());

    d.geo        = info.geo.get();
    d.material   = info.mat;

    DrawableMeta& m = drawableMetas.at(drawableId);
    m.pipelines  = getRequiredPipelines(info);
    for (auto p : m.pipelines) {
        addToPipeline(p, drawableId);
    }
}

void trc::RasterDrawablePool::deleteDrawable(ui32 drawableId)
{
    auto& m = drawableMetas.at(drawableId);

    for (auto p : m.pipelines) {
        removeFromPipeline(p, drawableId);
    }
    m.pipelines.clear();
}

void trc::RasterDrawablePool::createInstance(ui32 drawableId, const DrawableInstanceCreateInfo& info)
{
    auto& d = drawables.at(drawableId);

    std::scoped_lock lock(*d.instancesLock);
    d.instances.emplace_back(info.transform, info.animData);
}

void trc::RasterDrawablePool::deleteInstance(ui32 drawableId, ui32 instanceId)
{
    auto& d = drawables.at(drawableId);

    std::scoped_lock lock(*d.instancesLock);
    std::swap(d.instances.at(instanceId), d.instances.back());
    d.instances.pop_back();
}

auto trc::RasterDrawablePool::getRequiredPipelines(const DrawableCreateInfo& info)
    -> std::vector<Pipeline::ID>
{
    PipelineFeatureFlags flags;
    if (info.transparent)        flags |= PipelineFeatureFlagBits::eTransparent;
    if (info.geo.get().hasRig()) flags |= PipelineFeatureFlagBits::eAnimated;

    return { getPipeline(PipelineFeatureFlagBits::eShadow), getPipeline(flags) };
}

void trc::RasterDrawablePool::addToPipeline(Pipeline::ID pipeline, ui32 drawableId)
{
    auto& [drawVec, mutex] = drawCalls.at(pipeline);
    std::scoped_lock lock(mutex);
    util::insert_sorted(drawVec, drawableId);
}

void trc::RasterDrawablePool::removeFromPipeline(Pipeline::ID pipeline, ui32 drawableId)
{
    auto& [v, mutex] = drawCalls.at(pipeline);
    std::scoped_lock lock(mutex);
    v.erase(std::remove(v.begin(), v.end(), drawableId));
}

void trc::RasterDrawablePool::createRasterFunctions()
{
    auto makeFunction = [this](auto& pipelineData, auto func) {
        return [this, &pipelineData, func](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            std::scoped_lock lock(pipelineData.second);
            for (ui32 drawDataId : pipelineData.first)
            {
                auto& d = drawables.at(drawDataId);
                auto layout = env.currentPipeline->getLayout();

                d.geo.bindVertices(cmdBuf, 0);
                cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eVertex,
                                           sizeof(mat4), static_cast<ui32>(d.material));

                std::scoped_lock iLock(*d.instancesLock);
                for (auto& instance : d.instances)
                {
                    func(instance, env, cmdBuf);

                    cmdBuf.pushConstants<mat4>(layout, vk::ShaderStageFlagBits::eVertex, 0,
                                               instance.transform.get());
                    cmdBuf.drawIndexed(d.geo.getIndexCount(), 1, 0, 0, 0);
                }
            }
        };
    };

    auto addDrawFunc = [&](RenderStageType::ID stage, SubPass::ID subpass,
                           Pipeline::ID pipeline, auto func)
    {
        auto& drawVector = drawCalls.try_emplace(pipeline).first->second;
        drawFunctions.emplace(
            pipeline,
            DrawFunctionSpec{ makeFunction(drawVector, func), subpass, stage }
        );
    };

    // Default
    addDrawFunc(
        RenderStageTypes::getDeferred(), RenderPassDeferred::SubPasses::gBuffer,
        getPipeline({}),
        [](Instance&, auto&, vk::CommandBuffer) {}
    );

    // Animated
    addDrawFunc(
        RenderStageTypes::getDeferred(),
        RenderPassDeferred::SubPasses::gBuffer,
        getPipeline(PipelineFeatureFlagBits::eAnimated),
        [](Instance& inst, auto& env, vk::CommandBuffer cmdBuf) {
            cmdBuf.pushConstants<AnimationDeviceData>(
                env.currentPipeline->getLayout(), vk::ShaderStageFlagBits::eVertex,
                sizeof(mat4) + sizeof(ui32), inst.animData.get()
            );
        }
    );

    // Transparent default
    addDrawFunc(
        RenderStageTypes::getDeferred(), RenderPassDeferred::SubPasses::transparency,
        getPipeline(PipelineFeatureFlagBits::eTransparent),
        [](Instance&, auto&, vk::CommandBuffer) {}
    );

    addDrawFunc(
        RenderStageTypes::getShadow(),
        SubPass::ID(0),
        getPipeline(PipelineFeatureFlagBits::eShadow),
        [](Instance& inst, const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            auto currentRenderPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
            assert(currentRenderPass != nullptr);

            // Set pipeline dynamic states
            uvec2 res = currentRenderPass->getResolution();
            cmdBuf.setViewport(0, vk::Viewport(0.0f, 0.0f, res.x, res.y, 0.0f, 1.0f));
            cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { res.x, res.y }));

            // Bind buffers and push constants
            auto layout = env.currentPipeline->getLayout();
            cmdBuf.pushConstants<ui32>(
                layout, vk::ShaderStageFlagBits::eVertex,
                sizeof(mat4), currentRenderPass->getShadowMatrixIndex()
            );
            cmdBuf.pushConstants<AnimationDeviceData>(
                env.currentPipeline->getLayout(), vk::ShaderStageFlagBits::eVertex,
                sizeof(mat4) + sizeof(ui32), inst.animData.get()
            );
        }
    );
}
