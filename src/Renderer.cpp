#include "Renderer.h"

#include <vkb/util/Timer.h>
#include <vkb/ShaderProgram.h>

#include "PipelineDefinitions.h"
#include "PipelineRegistry.h"
#include "AssetRegistry.h"
#include "Particle.h" // For particle pipeline creation
#include "Scene.h"
#include "ComputePass.h"



/////////////////////////
//      Renderer       //
/////////////////////////

trc::Renderer::Renderer(RendererCreateInfo info)
    :
    // Create the deferred render pass
    defaultDeferredPass(RenderPass::createAtNextIndex<RenderPassDeferred>(
        vkb::getSwapchain(), info.maxTransparentFragsPerPixel
    ).first),
    aaPass(RenderPass::createAtNextIndex<ComputePass>().first),
    fullscreenQuadVertexBuffer(
        std::vector<vec3>{
            vec3(-1, 1, 0), vec3(1, 1, 0), vec3(-1, -1, 0),
            vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0)
        },
        vk::BufferUsageFlagBits::eVertexBuffer
    ),

    ///////////////////////////////
    //      Anti-aliasing        //
    ///////////////////////////////
    swapchainImageSamplers([](ui32) {
        return vkb::getDevice()->createSamplerUnique(vk::SamplerCreateInfo(
            {},
            vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat
        ));
    }),
    swapchainImageViews([](ui32 i) {
        return vkb::getDevice()->createImageViewUnique(
            vk::ImageViewCreateInfo(
                {},
                vkb::getSwapchain().getImage(i),
                vk::ImageViewType::e2D,
                vkb::getSwapchain().getImageFormat(),
                {}, vkb::DEFAULT_SUBRES_RANGE
            )
        );
    }),
    aaDescriptor(
        vkb::getSwapchain().getImageExtent(),
        [this]() {
            std::vector<std::pair<vk::Sampler, vk::ImageView>> result;
            for (size_t i = 0; i < vkb::getSwapchain().getFrameCount(); i++) {
                result.emplace_back(*swapchainImageSamplers.getAt(i), *swapchainImageViews.getAt(i));
            }
            return result;
        }()
    )
{
    enableRenderStageType(RenderStageTypes::eShadow, -10);
    enableRenderStageType(RenderStageTypes::eDeferred, 0);
    enableRenderStageType(RenderStageTypes::ePostProcessing, 1);
    addRenderStage(RenderStageTypes::eDeferred, defaultDeferredStage);
    addRenderStage(RenderStageTypes::ePostProcessing, aaStage);
    defaultDeferredStage.addRenderPass(defaultDeferredPass);
    aaStage.addRenderPass(aaPass);

    createSemaphores();

    // Pre recreate, finish rendering
    preRecreateListener = vkb::EventHandler<vkb::PreSwapchainRecreateEvent>::addListener(
        [this](const auto&) {
            waitForAllFrames();
        }
    );
    // Post recreate, create the required resources
    postRecreateListener = vkb::EventHandler<vkb::SwapchainRecreateEvent>::addListener(
        [this, maxFrags=info.maxTransparentFragsPerPixel](const auto&) {
            // Completely recreate the deferred renderpass
            RenderPass::replace<RenderPassDeferred>(
                defaultDeferredPass,
                vkb::getSwapchain(), maxFrags
            );
            PipelineRegistry::recreateAll();
        }
    );

    PipelineRegistry::registerPipeline([this]() { internal::makeAllDrawablePipelines(*this); });
    PipelineRegistry::registerPipeline([this]() { internal::makeFinalLightingPipeline(*this); });
    PipelineRegistry::registerPipeline([this]() {
        internal::makeParticleDrawPipeline(*this);
        internal::makeParticleShadowPipeline(*this);
    });
    // Create all pipelines for the first time
    PipelineRegistry::recreateAll();

    // Create anti-aliasing pipeline
    auto computeLayout = makePipelineLayout({ aaDescriptor.getDescriptorSetLayout() }, {});
    auto computeLayoutHandle = *computeLayout;
    makeComputePipeline(
        internal::Pipelines::eMorphologicalAntiAliasingCompute1,
        std::move(computeLayout),
        [computeLayoutHandle]() -> vk::UniquePipeline
        {
            auto shaderModule = vkb::createShaderModule(
                vkb::readFile(TRC_SHADER_DIR"/compute/mlaa_gen_edges.comp.spv")
            );
            return vkb::getDevice()->createComputePipelineUnique(
                {},
                vk::ComputePipelineCreateInfo(
                    {},
                    vk::PipelineShaderStageCreateInfo(
                        {}, vk::ShaderStageFlagBits::eCompute, *shaderModule, "main"
                    ),
                    computeLayoutHandle
                )
            ).value;
        }()
    );
    makeComputePipeline(
        internal::Pipelines::eMorphologicalAntiAliasingCompute2,
        std::move(computeLayout),
        [computeLayoutHandle]() -> vk::UniquePipeline
        {
            auto shaderModule = vkb::createShaderModule(
                vkb::readFile(TRC_SHADER_DIR"/compute/mlaa_calc_blend_weights.comp.spv")
            );
            return vkb::getDevice()->createComputePipelineUnique(
                {},
                vk::ComputePipelineCreateInfo(
                    {},
                    vk::PipelineShaderStageCreateInfo(
                        {}, vk::ShaderStageFlagBits::eCompute, *shaderModule, "main"
                    ),
                    computeLayoutHandle
                )
            ).value;
        }()
    );
    makeComputePipeline(
        internal::Pipelines::eMorphologicalAntiAliasingCompute3,
        std::move(computeLayout),
        [computeLayoutHandle]() -> vk::UniquePipeline
        {
            auto shaderModule = vkb::createShaderModule(
                vkb::readFile(TRC_SHADER_DIR"/compute/mlaa_blend_colors.comp.spv")
            );
            return vkb::getDevice()->createComputePipelineUnique(
                {},
                vk::ComputePipelineCreateInfo(
                    {},
                    vk::PipelineShaderStageCreateInfo(
                        {}, vk::ShaderStageFlagBits::eCompute, *shaderModule, "main"
                    ),
                    computeLayoutHandle
                )
            ).value;
        }()
    );
    vkb::on<vkb::KeyPressEvent>([this](const auto& e) {
        if (e.key == vkb::Key::a) {
            enableAntialiasing = !enableAntialiasing;
            std::cout << "Anti aliasing: " << (enableAntialiasing ? "on" : "off") << "\n";
        }
    });
}

trc::Renderer::~Renderer()
{
    waitForAllFrames();
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::getDevice();
    auto& swapchain = vkb::getSwapchain();

    // Update
    scene.update();
    sceneDescriptorProvider.setWrappedProvider(scene.getDescriptor().getProvider());
    shadowDescriptorProvider.setWrappedProvider(
        scene.getLightRegistry().getDescriptor().getProvider()
    );
    globalDataDescriptor.updateCameraMatrices(camera);
    globalDataDescriptor.updateSwapchainData(swapchain);

    // Add final lighting function to scene
    auto finalLightingFunc = scene.registerDrawFunction(
        RenderStageTypes::eDeferred,
        internal::DeferredSubPasses::eLightingPass,
        internal::Pipelines::eFinalLighting,
        [&](auto&&, vk::CommandBuffer cmdBuf)
        {
            cmdBuf.bindVertexBuffers(0, *fullscreenQuadVertexBuffer, vk::DeviceSize(0));
            cmdBuf.draw(6, 1, 0, 0);
        }
    );
    auto aaFunc = scene.registerDrawFunction(
        RenderStageTypes::ePostProcessing, 0,
        internal::Pipelines::eMorphologicalAntiAliasingCompute1,
        [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf) {
            executeAntiAliasing(env, cmdBuf);
        }
    );

    // Acquire image
    auto fenceResult = device->waitForFences(**frameInFlightFences, true, UINT64_MAX);
    assert(fenceResult == vk::Result::eSuccess);
    device->resetFences(**frameInFlightFences);
    auto image = swapchain.acquireImage(**imageAcquireSemaphores);

    // Collect commands
    std::vector<vk::CommandBuffer> cmdBufs;
    for (ui32 i = 0; const auto& [_, type, stages] : enabledStages)
    {
        // Additionally render all passes in the current scene's light
        // registry. This is not nice, but I wanted to move on to other
        // things.
        if (type == RenderStageTypes::eShadow)
        {
            cmdBufs.push_back(commandCollectors[i].recordScene(
                scene, type,
                scene.getLightRegistry().getShadowRenderStage()
            ));
            i++;
        }

        // Execute all render passes for this stage type
        for (RenderStage* stage : stages)
        {
            cmdBufs.push_back(commandCollectors[i].recordScene(scene, type, *stage));
            i++;
        }
    }

    // Remove fullscreen quad function
    scene.unregisterDrawFunction(finalLightingFunc);
    scene.unregisterDrawFunction(aaFunc);

    // Submit command buffers
    auto graphicsQueue = vkb::getDevice().getQueue(vkb::QueueType::graphics);
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eVertexInput;
    graphicsQueue.submit(
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            cmdBufs,
            **renderFinishedSemaphores
        ),
        **frameInFlightFences
    );

    // Present frame
    auto presentQueue = vkb::getDevice().getQueue(vkb::QueueType::presentation);
    swapchain.presentImage(image, presentQueue, { **renderFinishedSemaphores });
}

void trc::Renderer::enableRenderStageType(RenderStageType::ID stageType, i32 priority)
{
    auto it = enabledStages.begin();
    while (it != enabledStages.end() && it->priority < priority) it++;

    enabledStages.insert(it, EnabledStageType{ priority, stageType, {} });
    commandCollectors.emplace_back();
}

void trc::Renderer::addRenderStage(RenderStageType::ID type, RenderStage& stage)
{
    for (auto& stageType : enabledStages)
    {
        if (stageType.type == type)
        {
            stageType.stages.push_back(&stage);
            break;
        }
    }
}

auto trc::Renderer::getDefaultDeferredStage() const noexcept -> const DeferredStage&
{
    return defaultDeferredStage;
}

auto trc::Renderer::getDeferredRenderPass() const noexcept -> const RenderPassDeferred&
{
    return static_cast<RenderPassDeferred&>(RenderPass::at(defaultDeferredPass));
}

auto trc::Renderer::getGlobalDataDescriptor() const noexcept -> const GlobalRenderDataDescriptor&
{
    return globalDataDescriptor;
}

auto trc::Renderer::getGlobalDataDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return globalDataDescriptor.getProvider();
}

auto trc::Renderer::getSceneDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return sceneDescriptorProvider;
}

auto trc::Renderer::getShadowDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return shadowDescriptorProvider;
}

auto trc::Renderer::getMouseWorldPos(const Camera& camera) -> vec3
{
    return getMouseWorldPosAtDepth(camera, getDeferredRenderPass().getMouseDepth());
}

auto trc::Renderer::getMouseWorldPosAtDepth(const Camera& camera, const float depth) -> vec3
{
    assert(depth >= 0.0f && depth <= 1.0f);

    const auto windowSize = vkb::getSwapchain().getImageExtent();
    const vec2 mousePos = vkb::getSwapchain().getMousePosition();

    return glm::unProject(
        vec3(mousePos, depth),
        camera.getViewMatrix(),
        camera.getProjectionMatrix(),
        vec4(0.0f, 0.0f, windowSize.width, windowSize.height)
    );
}

void trc::Renderer::createSemaphores()
{
    imageAcquireSemaphores = { [](ui32) { return vkb::getDevice()->createSemaphoreUnique({}); }};
    renderFinishedSemaphores = { [](ui32) { return vkb::getDevice()->createSemaphoreUnique({}); }};
    frameInFlightFences = { [](ui32) {
        return vkb::getDevice()->createFenceUnique(
            { vk::FenceCreateFlagBits::eSignaled }
        );
    }};
}

void trc::Renderer::waitForAllFrames(ui64 timeoutNs)
{
    std::vector<vk::Fence> fences;
    for (auto& fence : frameInFlightFences) {
        fences.push_back(*fence);
    }
    auto result = vkb::VulkanBase::getDevice()->waitForFences(fences, true, timeoutNs);
    if (result == vk::Result::eTimeout) {
        std::cout << "Timeout in Renderer::waitForAllFrames!\n";
    }
}

void trc::Renderer::executeAntiAliasing(const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
{
    if (enableAntialiasing)
    {
        // Bring depth image in shader read layout
        const auto& swapchain = vkb::getSwapchain();
        const vk::Image currImage = swapchain.getImage(swapchain.getCurrentFrame());
        const vk::Extent2D size = swapchain.getImageExtent();

        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eAllGraphics,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::DependencyFlagBits::eByRegion,
            {}, {},
            vk::ImageMemoryBarrier(
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::ePresentSrcKHR,
                vk::ImageLayout::eGeneral,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                currImage, vkb::DEFAULT_SUBRES_RANGE
            )
        );

        aaDescriptor.bindDescriptorSet(
            cmdBuf, vk::PipelineBindPoint::eCompute,
            env.currentPipeline->getLayout(), 0 // set index
        );
        cmdBuf.dispatch(size.width / 8, size.height / 8, 1);

        Pipeline::at(internal::Pipelines::eMorphologicalAntiAliasingCompute2).bind(cmdBuf);
        aaDescriptor.bindDescriptorSet(
            cmdBuf, vk::PipelineBindPoint::eCompute,
            env.currentPipeline->getLayout(), 0 // set index
        );
        cmdBuf.dispatch(size.width / 8, size.height / 8, 1);

        Pipeline::at(internal::Pipelines::eMorphologicalAntiAliasingCompute3).bind(cmdBuf);
        aaDescriptor.bindDescriptorSet(
            cmdBuf, vk::PipelineBindPoint::eCompute,
            env.currentPipeline->getLayout(), 0 // set index
        );
        cmdBuf.dispatch(size.width / 8, size.height / 8, 1);

        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::DependencyFlagBits::eByRegion,
            {}, {},
            vk::ImageMemoryBarrier(
                {},
                vk::AccessFlagBits::eColorAttachmentRead,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::ePresentSrcKHR,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                currImage, vkb::DEFAULT_SUBRES_RANGE
            )
        );
    }
}



///////////////////////////////////////////
//      Descriptor provider wrapper      //
///////////////////////////////////////////

trc::Renderer::DescriptorProviderWrapper::DescriptorProviderWrapper(
    vk::DescriptorSetLayout staticLayout)
    :
    descLayout(staticLayout)
{
}

auto trc::Renderer::DescriptorProviderWrapper::getDescriptorSet() const noexcept
    -> vk::DescriptorSet
{
    if (provider != nullptr) {
        return provider->getDescriptorSet();
    }
    return {};
}

auto trc::Renderer::DescriptorProviderWrapper::getDescriptorSetLayout() const noexcept
    -> vk::DescriptorSetLayout
{
    return descLayout;
}

void trc::Renderer::DescriptorProviderWrapper::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    if (provider != nullptr) {
        provider->bindDescriptorSet(cmdBuf, bindPoint, pipelineLayout, setIndex);
    }
}

void trc::Renderer::DescriptorProviderWrapper::setWrappedProvider(
    const DescriptorProviderInterface& wrapped) noexcept
{
    provider = &wrapped;
}
