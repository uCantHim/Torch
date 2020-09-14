#include "Particle.h"

#include "PipelineDefinitions.cpp" // For the SHADER_DIR constant
#include "PipelineBuilder.h"
#include "RenderPass.h"



namespace trc::internal
{
    struct ParticleVertex
    {
        vec3 position;
        vec2 uv;
        vec3 normal;
    };
}

vkb::StaticInit trc::ParticleCollection::_init{
    []() {
        memoryPool.reset(new vkb::MemoryPool(vkb::getDevice(), 2000000)); // 2 MB

        PipelineRegistry::registerPipeline(internal::makeParticleDrawPipeline);
        PipelineRegistry::registerPipeline(internal::makeParticleShadowPipeline);

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
    persistentMaterialBuf(reinterpret_cast<ParticleMaterial*>(particleMaterialBuffer.map()))
{
    particles.reserve(maxParticles);
    setUpdateMethod(updateMethod);
}

void trc::ParticleCollection::attachToScene(SceneBase& scene)
{
    if (currentScene != nullptr) {
        removeFromScene();
    }

    sceneRegistrations.emplace_back(scene.registerDrawFunction(
        internal::RenderStages::eDeferred,
        internal::DeferredSubPasses::eTransparencyPass,
        internal::Pipelines::eParticleDraw,
        [this](const DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            if (particles.empty()) return;

            cmdBuf.bindVertexBuffers(0,
                { *vertexBuffer, *particleMatrixBuffer, *particleMaterialBuffer },
                { 0, 0, 0 });
            cmdBuf.draw(6, particles.size(), 0, 0);
        }
    ));
    //sceneRegistrations.emplace_back(scene.registerDrawFunction(
    //    internal::RenderStages::eShadow,
    //    0,
    //    internal::Pipelines::eParticleShadow,
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
    currentScene = &scene;
}

void trc::ParticleCollection::removeFromScene()
{
    assert(currentScene != nullptr);
    for (const auto& reg : sceneRegistrations) {
        currentScene->unregisterDrawFunction(reg);
    }
    sceneRegistrations.clear();
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

    // Run updater on matrix buffer
    auto matrixBuf = reinterpret_cast<mat4*>(particleMatrixStagingBuffer.map());
    updater->update(particles, matrixBuf, persistentMaterialBuf);
    particleMatrixStagingBuffer.unmap();

    // Copy matrices to vertex attribute buffer
    vkb::copyBuffer(
        vkb::getDevice(),
        *particleMatrixBuffer, *particleMatrixStagingBuffer,
        0, 0, particleMatrixStagingBuffer.size()
    );
}



////////////////////////////////
//      Particle Updater      //
////////////////////////////////

void trc::ParticleCollection::HostUpdater::update(
    std::vector<ParticlePhysical>& particles,
    mat4* transformData,
    ParticleMaterial* materialData)
{
    const float frameTimeMs = float(frameTimer.reset()) / 1000.0f;
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

void trc::internal::makeParticleDrawPipeline()
{
    auto& renderPass = RenderPass::at(RenderPasses::eDeferredPass);

    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>{
            GlobalRenderDataDescriptor::getProvider().getDescriptorSetLayout(),
            AssetRegistry::getDescriptorSetProvider().getDescriptorSetLayout(),
            DeferredRenderPassDescriptor::getProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{}
    );

    vkb::ShaderProgram program(SHADER_DIR / "particles/deferred.vert.spv",
                               SHADER_DIR / "particles/deferred.frag.spv");

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
        .build(*vkb::getDevice(), *layout, *renderPass, DeferredSubPasses::eTransparencyPass);

    auto& p = makeGraphicsPipeline(
        Pipelines::eParticleDraw,
        std::move(layout),
        std::move(pipeline));

    p.addStaticDescriptorSet(0, GlobalRenderDataDescriptor::getProvider());
    p.addStaticDescriptorSet(1, AssetRegistry::getDescriptorSetProvider());
    p.addStaticDescriptorSet(2, DeferredRenderPassDescriptor::getProvider());
}

void trc::internal::makeParticleShadowPipeline()
{
    RenderPassShadow dummyPass({ 1, 1 }, {});

    auto layout = makePipelineLayout(
        std::vector<vk::DescriptorSetLayout>{
            ShadowDescriptor::getProvider().getDescriptorSetLayout(),
            GlobalRenderDataDescriptor::getProvider().getDescriptorSetLayout(),
        },
        std::vector<vk::PushConstantRange>{
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(ui32)),
        }
    );

    vkb::ShaderProgram program(SHADER_DIR / "particles/shadow.vert.spv",
                               SHADER_DIR / "particles/shadow.frag.spv");

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
        .build(*vkb::getDevice(), *layout, *dummyPass, 0);

    auto& p = makeGraphicsPipeline(
        Pipelines::eParticleShadow,
        std::move(layout),
        std::move(pipeline));

    p.addStaticDescriptorSet(0, ShadowDescriptor::getProvider());
    p.addStaticDescriptorSet(1, GlobalRenderDataDescriptor::getProvider());
}
