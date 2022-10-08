#include "trc/particle/Particle.h"

#include "trc/core/Instance.h"

#include "trc/core/PipelineLayoutBuilder.h"
#include "trc/core/PipelineBuilder.h"
#include "trc/TorchRenderConfig.h"
#include "trc/PipelineDefinitions.h" // For the SHADER_DIR constant
#include "trc/TorchResources.h"
#include "trc/ParticlePipelines.h"



namespace trc
{
    struct ParticleVertex
    {
        vec3 position;
        vec2 uv;
        vec3 normal;
    };
}



trc::ParticleCollection::ParticleCollection(
    Instance& instance,
    const ui32 maxParticles)
    :
    instance(instance),
    maxParticles(maxParticles),
    memoryPool(instance.getDevice(), 2000000), // 2 MB
    vertexBuffer(
        instance.getDevice(),
        std::vector<ParticleVertex>{
            { vec3(-0.1f, -0.1f, 0.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
            { vec3( 0.1f,  0.1f, 0.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
            { vec3(-0.1f,  0.1f, 0.0f), vec2(0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
            { vec3(-0.1f, -0.1f, 0.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
            { vec3( 0.1f, -0.1f, 0.0f), vec2(1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
            { vec3( 0.1f,  0.1f, 0.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
        },
        vk::BufferUsageFlagBits::eVertexBuffer,
        memoryPool.makeAllocator()
    ),
    particleDeviceDataStagingBuffer(
        instance.getDevice(),
        sizeof(ParticleDeviceData) * maxParticles,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible,
        memoryPool.makeAllocator()
    ),
    particleDeviceDataBuffer(
        instance.getDevice(),
        sizeof(ParticleDeviceData) * maxParticles, nullptr,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        memoryPool.makeAllocator()
    ),
    persistentParticleDeviceDataBuf(
        reinterpret_cast<ParticleDeviceData*>(particleDeviceDataStagingBuffer.map())
    ),
    transferFence(instance.getDevice()->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled }))
{
    auto& dev = instance.getDevice();
    auto [tq, tf] = dev.getQueueManager().getAnyQueue(QueueType::transfer);
    transferQueue = dev.getQueueManager().reserveQueue(tq);
    transferCmdPool = dev->createCommandPoolUnique({
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, tf
    });
    transferCmdBuf = std::move(dev->allocateCommandBuffersUnique({
        *transferCmdPool, vk::CommandBufferLevel::ePrimary, 1
    })[0]);

    particles.reserve(maxParticles);
}

void trc::ParticleCollection::attachToScene(SceneBase& scene)
{
    // Alpha discard pipeline
    drawRegistrations[Blend::eDiscardZeroAlpha] = scene.registerDrawFunction(
        gBufferRenderStage,
        GBufferPass::SubPasses::gBuffer,
        pipelines::particle::getParticlePipeline(pipelines::particle::AlphaBlendTypeFlagBits::discard),
        [this](const DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            auto [offset, count] = blendTypeSizes[Blend::eDiscardZeroAlpha];
            if (count == 0) return;

            cmdBuf.bindVertexBuffers(0,
                { *vertexBuffer, *particleDeviceDataBuffer },
                { 0, offset * sizeof(ParticleDeviceData) });
            cmdBuf.draw(6, count, 0, 0);
        }
    ).makeUnique();

    // Alpha blend pipeline
    drawRegistrations[Blend::eAlphaBlend] = scene.registerDrawFunction(
        gBufferRenderStage,
        GBufferPass::SubPasses::transparency,
        pipelines::particle::getParticlePipeline(pipelines::particle::AlphaBlendTypeFlagBits::blend),
        [this](const DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            auto [offset, count] = blendTypeSizes[Blend::eAlphaBlend];
            if (count == 0) return;

            cmdBuf.bindVertexBuffers(0,
                { *vertexBuffer, *particleDeviceDataBuffer },
                { 0, offset * sizeof(ParticleDeviceData) });
            cmdBuf.draw(6, count, 0, 0);
        }
    ).makeUnique();

    //shadowRegistration = scene.registerDrawFunction(
    //    shadowRenderStage, SubPass::ID(0), getShadowPipeline(),
    //    [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
    //    {
    //        if (particles.empty()) return;

    //        auto shadowPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
    //        assert(shadowPass != nullptr);

    //        cmdBuf.pushConstants<ui32>(
    //            env.currentPipeline->getLayout(),
    //            vk::ShaderStageFlagBits::eVertex,
    //            0, shadowPass->getShadowMatrixIndex());
    //        cmdBuf.bindVertexBuffers(0, { *vertexBuffer, *particleDeviceDataBuffer }, { 0, 0 });
    //        cmdBuf.draw(6, particles.size(), 0, 0);
    //    }
    //);
}

void trc::ParticleCollection::removeFromScene()
{
    drawRegistrations = {};
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

void trc::ParticleCollection::update(const float timeDelta)
{
    // Add new particles
    {
        std::lock_guard lock(newParticleListLock);
        for (const auto& p : newParticles)
        {
            if (particles.size() >= maxParticles) {
                break;
            }
            particles.push_back(p);
        }
        newParticles.clear();
    }

    if (particles.empty()) return;

    // Transfer from staging to main buffer must be completed before the
    // updater writes to the staging buffer again.
    [[maybe_unused]]
    auto result = instance.getDevice()->waitForFences(*transferFence, true, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
    instance.getDevice()->resetFences(*transferFence);

    // Run updater on matrix buffer
    tickParticles(timeDelta);

    // Copy matrices to vertex attribute buffer
    transferCmdBuf->begin(vk::CommandBufferBeginInfo());
    transferCmdBuf->copyBuffer(
        *particleDeviceDataStagingBuffer, *particleDeviceDataBuffer,
        vk::BufferCopy(0, 0, particleDeviceDataStagingBuffer.size())
    );
    transferCmdBuf->end();

    constexpr vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eTransfer;
    transferQueue.waitSubmit(
        vk::SubmitInfo(0, nullptr, &waitStage, 1, &*transferCmdBuf, 0, nullptr),
        *transferFence
    );
}

void trc::ParticleCollection::tickParticles(const float timeDelta)
{
    const float frameTimeMs = timeDelta;

    PerBlendType<BlendTypeSize> tmpSizes;
    for (size_t i = 0; i < particles.size(); /* nothing */)
    {
        auto& p = particles[i].phys;

        // Calculate particle lifetime
        if (p.lifeTime - p.timeLived <= 0.0f)
        {
            // Particle is dead. Use the classic unstable remove.
            particles[i] = particles.back();
            particles.pop_back();
            continue;
        }
        p.timeLived += frameTimeMs;

        const float secondsElapsed = timeDelta / 1000.0f;

        // Simulate particle physics
        p.linearVelocity += secondsElapsed * p.linearAcceleration;
        p.position += secondsElapsed * p.linearVelocity;

        const quat rot = glm::angleAxis(secondsElapsed * p.angularVelocity, p.rotationAxis);
        p.orientation = rot * p.orientation;

        ++tmpSizes[particles[i].material.blending].count;

        ++i;
    }

    tmpSizes[Blend::eDiscardZeroAlpha].offset = 0;
    tmpSizes[Blend::eAlphaBlend].offset       = tmpSizes[Blend::eDiscardZeroAlpha].count;
    blendTypeSizes = tmpSizes;

    ParticleDeviceData* devData = persistentParticleDeviceDataBuf;
    for (size_t i = 0; i < particles.size(); i++)
    {
        const ui32 index = tmpSizes[particles[i].material.blending].offset++;

        // Store calculated transform in buffer
        auto& p = particles[i].phys;
        devData[index].transform = glm::translate(mat4(1.0f), p.position)
                                   * mat4(p.orientation)
                                   * glm::scale(mat4(1.0f), p.scaling);
        devData[index].textureIndex = particles[i].material.texture;
    }
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
