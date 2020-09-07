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
        vertexBuffer = vkb::DeviceLocalBuffer(
            std::vector<ParticleVertex>{
                { vec3(-0.1f, -0.1f, 0.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3( 0.1f,  0.1f, 0.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3(-0.1f,  0.1f, 0.0f), vec2(0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3(-0.1f, -0.1f, 0.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3( 0.1f, -0.1f, 0.0f), vec2(1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
                { vec3( 0.1f,  0.1f, 0.0f), vec2(1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) },
            },
            vk::BufferUsageFlagBits::eVertexBuffer
        );
    },
    []() { vertexBuffer = {}; }
};

static_assert(sizeof(trc::ParticleMaterial) == util::sizeof_pad_16_v<trc::ParticleMaterial>,
              "ParticleMaterial must be padded to a multiple of 16 bytes!");



trc::ParticleCollection::ParticleCollection(
    const ui32 maxParticles,
    ParticleUpdateMethod updateMethod)
    :
    maxParticles(maxParticles),
    particleMatrixStagingBuffer(
        sizeof(mat4) * maxParticles,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    particleMatrixBuffer(
        sizeof(mat4) * maxParticles, nullptr,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    ),
    particleMaterialBuffer(
        util::sizeof_pad_16_v<ParticleMaterial> * maxParticles, nullptr,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    )
{
    setUpdateMethod(updateMethod);
}

void trc::ParticleCollection::attachToScene(SceneBase& scene)
{
    if (currentScene != nullptr) {
        removeFromScene();
    }

    sceneRegistrations.emplace_back(scene.registerDrawFunction(
        internal::RenderStages::eDeferred,
        internal::DeferredSubPasses::eGBufferPass,
        internal::Pipelines::eParticleDraw,
        [this](const DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            if (particles.empty()) return;
            cmdBuf.bindVertexBuffers(0, { *vertexBuffer, *particleMatrixBuffer }, { 0, 0 });
            cmdBuf.draw(6, particles.size(), 0, 0);
        }
    ));
    sceneRegistrations.emplace_back(scene.registerDrawFunction(
        internal::RenderStages::eShadow,
        0,
        internal::Pipelines::eParticleShadow,
        [this](const DrawEnvironment& env, vk::CommandBuffer cmdBuf)
        {
            if (particles.empty()) return;

            auto shadowPass = dynamic_cast<RenderPassShadow*>(env.currentRenderPass);
            assert(shadowPass != nullptr);

            cmdBuf.pushConstants<ui32>(
                env.currentPipeline->getLayout(),
                vk::ShaderStageFlagBits::eVertex,
                0, shadowPass->getShadowIndex());
            cmdBuf.bindVertexBuffers(0, { *vertexBuffer, *particleMatrixBuffer }, { 0, 0 });
            cmdBuf.draw(6, particles.size(), 0, 0);
        }
    ));
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
    std::lock_guard lock(lockParticleUpdate);
    if (particles.size() < maxParticles)
    {
        particles.push_back(particle.phys);
        materials.push_back(particle.material);
    }
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
    std::lock_guard lock(lockParticleUpdate);

    // Run updater on matrix buffer
    auto matrixBuf = reinterpret_cast<mat4*>(particleMatrixStagingBuffer.map());
    updater->update(particles, matrixBuf);
    particleMatrixStagingBuffer.unmap();

    // Copy matrices to vertex attribute buffer
    auto cmdBuf = vkb::getDevice().createTransferCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo());
    cmdBuf->copyBuffer(
        *particleMatrixStagingBuffer, *particleMatrixBuffer,
        vk::BufferCopy(0, 0, particleMatrixStagingBuffer.size())
    );
    cmdBuf->end();
    vkb::getDevice().executeTransferCommandBufferSyncronously(*cmdBuf);
}



////////////////////////////////
//      Particle Updater      //
////////////////////////////////

void trc::ParticleCollection::HostUpdater::update(
    std::vector<ParticlePhysical>& particles,
    mat4* transformData)
{
    const float frameTimeMs = float(frameTimer.reset()) / 1000.0f;
    const float frameTimeSec = frameTimeMs / 1000.0f;
    for (size_t i = 0; i < particles.size(); /* nothing */)
    {
        auto& p = particles[i];

        // Calculate particle lifetime
        if (p.lifeTime - p.timeLived <= 0.0f)
        {
            p = particles.back();
            particles.pop_back();
            continue;
        }
        p.timeLived += frameTimeMs;

        // Simulate particle physics
        p.position += frameTimeSec * p.linearVelocity;
        const quat rotDelta = glm::angleAxis(p.angularVelocity * frameTimeSec, p.rotationAxis);
        p.orientation = rotDelta * p.orientation;

        // Store calculated transform in buffer
        transformData[i++] = glm::translate(mat4(1.0f), p.position) * mat4(p.orientation);
    }
}

void trc::ParticleCollection::DeviceUpdater::update(
    std::vector<ParticlePhysical>&,
    mat4*)
{

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
        //.addVertexInputBinding(
        //    vk::VertexInputBindingDescription(2, sizeof(ParticleMaterial),
        //                                      vk::VertexInputRate::eInstance),
        //    {
        //        { 7, 1, vk::Format::eR32Uint, 0 },                      // emitting
        //        { 8, 1, vk::Format::eR32Uint, sizeof(bool32) },         // hasShadow
        //        { 9, 1, vk::Format::eR32Uint, sizeof(bool32) * 2 },     // texture
        //    }
        //)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .addViewport(vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f))
        .addScissorRect({ { 0, 0 }, extent })
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .build(*vkb::getDevice(), *layout, *renderPass, DeferredSubPasses::eGBufferPass);

    auto& p = makeGraphicsPipeline(
        Pipelines::eParticleDraw,
        std::move(layout),
        std::move(pipeline));

    p.addStaticDescriptorSet(0, GlobalRenderDataDescriptor::getProvider());
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
