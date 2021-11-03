#include "drawable/RasterDrawablePool.h"

#include "util/algorithm/VectorTransform.h"
#include "TorchResources.h"
#include "RenderPassDeferred.h"
#include "RenderPassShadow.h"
#include "PipelineDefinitions.h"
#include "drawable/RasterPipelines.h"



trc::RasterDrawablePool::RasterDrawablePool(const vkb::Device& device, const RasterDrawablePoolCreateInfo& info)
    :
    drawables(info.maxDrawables),
    renderData(info.maxDrawables),
    instanceDataBuffer(
        device,
        sizeof(InstanceGpuData) * info.maxDrawables,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible
    ),
    instanceDataBufferMap(reinterpret_cast<InstanceGpuData*>(instanceDataBuffer.map()))
{
    createRasterFunctions();
}

void trc::RasterDrawablePool::update()
{
    std::scoped_lock lock(renderDataLock);

    ui32 instanceIndex{ 0 };
    for (ui32 i = 0; i < drawables.size(); i++)
    {
        const auto& d = drawables.at(i);
        if (d.instances.empty()) break;

        renderData[i].instanceOffset = sizeof(InstanceGpuData) * instanceIndex;

        std::scoped_lock iLock(*d.instancesLock);
        for (const auto& inst : d.instances)
        {
            if (sizeof(InstanceGpuData) * instanceIndex >= instanceDataBuffer.size()) break;

            instanceDataBufferMap[instanceIndex].model = inst.transform.get();
            auto anim = inst.animData.get();
            instanceDataBufferMap[instanceIndex].animData = uvec4(
                anim.currentAnimation,
                anim.keyframes[0],
                anim.keyframes[1],
                glm::floatBitsToUint(anim.keyframeWeight)
            );
            ++instanceIndex;
        }
        renderData[i].instanceCount = d.instances.size();
    }
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
    assert(drawables.at(drawableId).instances.empty());

    renderData[drawableId].geo = info.geo.get();
    renderData[drawableId].mat = info.mat;

    DrawableData& d = drawables.at(drawableId);
    d.pipelines = getRequiredPipelines(info);
    for (auto p : d.pipelines) {
        addToPipeline(p, drawableId);
    }
}

void trc::RasterDrawablePool::deleteDrawable(ui32 drawableId)
{
    auto& d = drawables.at(drawableId);
    assert(d.instances.empty());

    for (auto p : d.pipelines) {
        removeFromPipeline(p, drawableId);
    }
    d.pipelines.clear();
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
    assert(d.instances.size() > instanceId);

    std::scoped_lock lock(*d.instancesLock);
    std::swap(d.instances.at(instanceId), d.instances.back());
    d.instances.pop_back();
}

auto trc::RasterDrawablePool::getRequiredPipelines(const DrawableCreateInfo& info)
    -> std::vector<Pipeline::ID>
{
    PipelineFeatureFlags flags;
    if (info.transparent) flags |= PipelineFeatureFlagBits::eTransparent;

    return {
        getPoolInstancePipeline(PipelineFeatureFlagBits::eShadow),
        getPoolInstancePipeline(flags)
    };
}

void trc::RasterDrawablePool::addToPipeline(Pipeline::ID pipeline, ui32 drawableId)
{
    assert(drawCalls.contains(pipeline));

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
            std::scoped_lock lock(pipelineData.second, renderDataLock);
            for (ui32 drawDataId : pipelineData.first)
            {
                auto& data = renderData.at(drawDataId);
                auto layout = env.currentPipeline->getLayout();

                data.geo.bindVertices(cmdBuf, 0);
                cmdBuf.bindVertexBuffers(1, *instanceDataBuffer, data.instanceOffset);
                cmdBuf.pushConstants<ui32>(layout, vk::ShaderStageFlagBits::eVertex,
                                           0, static_cast<ui32>(data.mat));

                func(data, env, cmdBuf);

                cmdBuf.drawIndexed(data.geo.getIndexCount(), data.instanceCount, 0, 0, 0);
            }
        };
    };

    auto addDrawFunc = [&](RenderStage::ID stage, SubPass::ID subpass,
                           Pipeline::ID pipeline, auto func)
    {
        auto& drawVector = drawCalls.try_emplace(pipeline).first->second;
        drawFunctions.emplace(
            pipeline,
            DrawFunctionSpec{ makeFunction(drawVector, func), subpass, stage }
        );
    };

    addDrawFunc(
        shadowRenderStage, SubPass::ID(0),
        getPoolInstancePipeline(PipelineFeatureFlagBits::eShadow),
        [](RenderData&, const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
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
                0, currentRenderPass->getShadowMatrixIndex()
            );
        }
    );

    // Instanced draw function
    addDrawFunc(
        deferredRenderStage, RenderPassDeferred::SubPasses::gBuffer,
        getPoolInstancePipeline({}),
        [](auto&, auto&, auto&) {}
    );

    addDrawFunc(
        deferredRenderStage, RenderPassDeferred::SubPasses::transparency,
        getPoolInstancePipeline(PipelineFeatureFlagBits::eTransparent),
        [](auto&, auto&, auto&) {}
    );
}
