#include "Particle.h"

#include "AssetRegistry.h"
#include "PipelineDefinitions.h" // For the SHADER_DIR constant
#include "PipelineBuilder.h"
#include "Renderer.h"



namespace trc
{
    struct ParticleVertex
    {
        vec3 position;
        vec2 uv;
        vec3 normal;
    };

    auto makeParticleDrawPipeline(vk::RenderPass deferredPass) -> trc::Pipeline;
    auto makeParticleShadowPipeline(vk::RenderPass deferredPass) -> trc::Pipeline;
}



vkb::StaticInit trc::ParticleCollection::_init{
    []() {
        memoryPool.reset(new vkb::MemoryPool(vkb::getDevice(), 2000000)); // 2 MB

        vertexBuffer = vkb::DeviceLocalBuffer(
            std::vector<ParticleVertex>{
                { vec3(-0.1f, -0.1f, 0.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3( 0.1f,  0.1f, 0.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3(-0.1f,  0.1f, 0.0f), vec2(0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3(-0.1f, -0.1f, 0.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3( 0.1f, -0.1f, 0.0f), vec2(1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3( 0.1f,  0.1f, 0.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
            },
            vk::BufferUsageFlagBits::eVertexBuffer,
            memoryPool->makeAllocator()
        );
    },
    []() {
        vertexBuffer = {};
        memoryPool.reset();
    }
};

trc::ParticleCollection::ParticleCollection(
    const ui32 maxParticles,
    ParticleUpdateMethod updateMethod)
    :
    maxParticles(maxParticles),
    particleMatrixStagingBuffer(
        sizeof(mat4) * maxParticles,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible,
        memoryPool->makeAllocator()
    ),
    particleMatrixBuffer(
        sizeof(mat4) * maxParticles, nullptr,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        memoryPool->makeAllocator()
    ),
    particleMaterialBuffer(
        sizeof(ParticleMaterial) * maxParticles, nullptr,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
        memoryPool->makeAllocator()
    ),
    persistentMaterialBuf(reinterpret_cast<ParticleMaterial*>(particleMaterialBuffer.map())),
    transferFence(vkb::getDevice()->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled })),
    transferCmdBuf(vkb::getDevice().createTransferCommandBuffer())
{
    particles.reserve(maxParticles);
    setUpdateMethod(updateMethod);
}

void trc::ParticleCollection::attachToScene(SceneBase& scene)
{
    drawRegistration = scene.registerDrawFunction(
        RenderStageTypes::getDeferred(),
        internal::DeferredSubPasses::transparencyPass,
        getDeferredPipeline(),
        [this](const DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            if (particles.empty()) return;

            cmdBuf.bindVertexBuffers(0,
                { *vertexBuffer, *particleMatrixBuffer, *particleMaterialBuffer },
                { 0, 0, 0 });
            cmdBuf.draw(6, particles.size(), 0, 0);
        }
    ).makeUnique();
    //sceneRegistrations.emplace_back(scene.registerDrawFunction(
    //    internal::RenderStageTypes::eShadow,
    //    0,
    //    getShadowPipeline(),
    //    [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
    //    {
    //        if (particles.empty()) return;

    //        auto shadowPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
    //        assert(shadowPass != nullptr);

    //        cmdBuf.pushConstants<ui32>(
    //            env.currentPipeline->getLayout(),
    //            vk::ShaderStageFlagBits::eVertex,
    //            0, shadowPass->getShadowIndex());
    //        cmdBuf.bindVertexBuffers(0, { *vertexBuffer, *particleMatrixBuffer }, { 0, 0 });
    //        cmdBuf.draw(6, particles.size(), 0, 0);
    //    }
    //));
}

void trc::ParticleCollection::removeFromScene()
{
    drawRegistration = {};
}

void trc::ParticleCollection::addParticle(const Particle& particle)
{
    std::lock_guard lock(newParticleListLock);
    newParticles.push_back(particle);
}

void trc::ParticleCollection::addParticles(const std::vector<Particle>& _newParticles)
{
    std::lock_guard lock(newParticleListLock);
    newParticles.insert(newParticles.end(), _newParticles.begin(), _newParticles.end());
}

void trc::ParticleCollection::setUpdateMethod(ParticleUpdateMethod method)
{
    switch (method)
    {
    case ParticleUpdateMethod::eDevice:
        updater.reset(new DeviceUpdater);
        break;
    case ParticleUpdateMethod::eHost:
        updater.reset(new HostUpdater);
        break;
    default:
        throw std::logic_error("Unknown enum ParticleUpdateMethod");
    }
}

void trc::ParticleCollection::update()
{
    // Add new particles
    std::unique_lock lock(newParticleListLock);
    for (const auto& p : newParticles)
    {
        if (particles.size() >= maxParticles) {
            break;
        }

        persistentMaterialBuf[particles.size()] = p.material;
        particles.push_back(p.phys);
    }
    newParticles.clear();
    lock.unlock();
    if (particles.empty()) return;

    // Transfer from staging to main buffer must be completed before the
    // updater writes to the staging buffer again.
    auto result = vkb::getDevice()->waitForFences(*transferFence, true, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
    vkb::getDevice()->resetFences(*transferFence);

    // Run updater on matrix buffer
    auto matrixBuf = reinterpret_cast<mat4*>(particleMatrixStagingBuffer.map());
    updater->update(particles, matrixBuf, persistentMaterialBuf);
    particleMatrixStagingBuffer.unmap();

    // Copy matrices to vertex attribute buffer
    transferCmdBuf->begin(vk::CommandBufferBeginInfo());
    transferCmdBuf->copyBuffer(
        *particleMatrixStagingBuffer, *particleMatrixBuffer,
        vk::BufferCopy(0, 0, particleMatrixStagingBuffer.size())
    );
    transferCmdBuf->end();

    constexpr vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eTransfer;
    static auto& transferQueue = Queues::getTransfer();
    transferQueue.waitSubmit(
        vk::SubmitInfo(0, nullptr, &waitStage, 1, &*transferCmdBuf, 0, nullptr),
        *transferFence
    );
}

auto trc::ParticleCollection::getDeferredPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        auto renderPass = RenderPassDeferred::makeVkRenderPassInstance(vkb::getSwapchain());
        return makeParticleDrawPipeline(*renderPass);
    });

    return id;
}

auto trc::ParticleCollection::getShadowPipeline() -> Pipeline::ID
{
    static auto id = PipelineRegistry::registerPipeline([] {
        RenderPassShadow shadowPass{ { 1, 1 } };
        return makeParticleShadowPipeline(*shadowPass);
    });

    return id;
}




////////////////////////////////
//      Particle Updater      //
////////////////////////////////

void trc::ParticleCollection::HostUpdater::update(
    std::vector<ParticlePhysical>& particles,
    mat4* transformData,
    ParticleMaterial* materialData)
{
    const float frameTimeMs = frameTimer.reset();
    for (size_t i = 0; i < particles.size(); /* nothing */)
    {
        auto& p = particles[i];

        // Calculate particle lifetime
        if (p.lifeTime - p.timeLived <= 0.0f)
        {
            if (p.doRespawn) {
                p.timeLived = 0.0f;
            }
            else
            {
                p = particles.back();
                particles.pop_back();
                // Rearrange materials accordingly
                materialData[i] = materialData[particles.size()];
                continue;
            }
        }
        p.timeLived += frameTimeMs;
        const float secondsLived = p.timeLived / 1000.0f;

        // Simulate particle physics
        const vec3 pos = p.position
                         + secondsLived * p.linearVelocity
                         + secondsLived * secondsLived * p.linearAcceleration * 0.5f;
        const quat rot = glm::angleAxis(secondsLived * p.angularVelocity, p.rotationAxis);
        const quat orientation = rot * p.orientation;

        // Store calculated transform in buffer
        transformData[i++] = glm::translate(mat4(1.0f), pos)
                             * mat4(orientation)
                             * glm::scale(mat4(1.0f), p.scaling);
    }
}

void trc::ParticleCollection::DeviceUpdater::update(
    std::vector<ParticlePhysical>&,
    mat4*,
    ParticleMaterial*)
{

}



//////////////////////////////
//      Particle Spawn      //
//////////////////////////////

trc::ParticleSpawn::ParticleSpawn(ParticleCollection& c, std::vector<Particle> particles)
    :
    particles(std::move(particles)),
    collection(&c)
{
}

void trc::ParticleSpawn::addParticle(Particle particle)
{
    particles.push_back(particle);
}

void trc::ParticleSpawn::spawnParticles()
{
    threads.async([this]()
    {
        const mat4& globalTransform = getGlobalTransform();
        std::vector<Particle> newParticles{ particles };
        for (auto& p : newParticles)
        {
            p.phys.position = vec3(globalTransform * vec4(p.phys.position, 1.0f));
            p.phys.timeLived = 0.0f;
        }

        // This function is costly as well
        collection->addParticles(particles);
    });
}



//////////////////////////////////////
//      Particle Draw Pipeline      //
//////////////////////////////////////

auto trc::makeParticleDrawPipeline(vk::RenderPass deferredPass) -> Pipeline
{
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>{
            Renderer::getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            Renderer::getDeferredPassDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{}
    );

    vkb::ShaderProgram program(internal::SHADER_DIR / "particles/deferred.vert.spv",
                               internal::SHADER_DIR / "particles/deferred.frag.spv");

    auto extent = vkb::getSwapchain().getImageExtent();
    auto pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        // (per-vertex) Vertex positions
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(ParticleVertex),
                                              vk::VertexInputRate::eVertex),
            {
                { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
                { 1, 0, vk::Format::eR32G32Sfloat,    sizeof(vec3) },
                { 2, 0, vk::Format::eR32G32B32Sfloat, sizeof(vec3) + sizeof(vec2) },
            }
        )
        // (per-instance) Particle model matrix
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(1, sizeof(mat4), vk::VertexInputRate::eInstance),
            {
                { 3, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 0 },
                { 4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 1 },
                { 5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 2 },
                { 6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 3 },
            }
        )
        // (per-instance) Draw properties
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(2, sizeof(ParticleMaterial),
                                              vk::VertexInputRate::eInstance),
            {
                { 7, 1, vk::Format::eR32Uint, 0 }, // texture
            }
        )
        .setCullMode(vk::CullModeFlagBits::eNone)
        .disableDepthWrite()
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect({ { 0, 0 }, extent })
        .addDynamicState(vk::DynamicState::eViewport)
        .build(*vkb::getDevice(), *layout, deferredPass, internal::DeferredSubPasses::transparencyPass);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, Renderer::getGlobalDataDescriptorProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, Renderer::getDeferredPassDescriptorProvider());

    return p;
}

auto trc::makeParticleShadowPipeline(vk::RenderPass shadowPass) -> Pipeline
{
    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>{
            Renderer::getShadowDescriptorProvider().getDescriptorSetLayout(),
            Renderer::getGlobalDataDescriptorProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(ui32)),
        }
    );

    vkb::ShaderProgram program(internal::SHADER_DIR / "particles/shadow.vert.spv",
                               internal::SHADER_DIR / "particles/shadow.frag.spv");

    auto pipeline = GraphicsPipelineBuilder::create()
        .setProgram(program)
        // (per-vertex) Vertex positions
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(0, sizeof(ParticleVertex),
                                              vk::VertexInputRate::eVertex),
            {
                { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
                { 1, 0, vk::Format::eR32G32Sfloat,    sizeof(vec3) },
                { 2, 0, vk::Format::eR32G32B32Sfloat, sizeof(vec3) + sizeof(vec2) },
            }
        )
        // (per-instance) Particle model matrix
        .addVertexInputBinding(
            vk::VertexInputBindingDescription(1, sizeof(mat4), vk::VertexInputRate::eInstance),
            {
                { 3, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 0 },
                { 4, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 1 },
                { 5, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 2 },
                { 6, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(vec4) * 3 },
            }
        )
        .setCullMode(vk::CullModeFlagBits::eNone)
        .addViewport(vk::Viewport(0, 0, 1, 1, 0.0f, 1.0f))
        .addScissorRect({ { 0, 0 }, { 1, 1 } })
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(*vkb::getDevice(), *layout, shadowPass, 0);

    Pipeline p{ std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eGraphics };
    p.addStaticDescriptorSet(0, Renderer::getShadowDescriptorProvider());
    p.addStaticDescriptorSet(1, Renderer::getGlobalDataDescriptorProvider());

    return p;
}
