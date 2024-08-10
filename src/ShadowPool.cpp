#include "trc/ShadowPool.h"

#include "trc/ray_tracing/RayPipelineBuilder.h"



trc::ShadowPool::Shadow::Shadow(
    const Device& device,
    const FrameClock& clock,
    ui32 index,
    uvec2 size)
    :
    index(index),
    camera(std::make_shared<Camera>()),
    renderPass(std::make_shared<RenderPassShadow>(
        device,
        clock,
        ShadowPassCreateInfo{ .shadowIndex=index, .resolution=size }
    ))
{
}



trc::ShadowPool::ShadowPool(
    const Device& device,
    const FrameClock& clock,
    ShadowPoolCreateInfo info)
    :
    device(device),
    clock(clock),
    kMaxShadowMaps(info.maxShadowMaps),
    // Must be a storage buffer rather than a uniform buffer because it has
    // a dynamically sized array in GLSL.
    shadowMatrixBuffer(
        device, sizeof(mat4) * info.maxShadowMaps,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
        | vk::MemoryPropertyFlagBits::eHostCoherent
    ),
    shadowMatrixBufferMap(shadowMatrixBuffer.map<mat4*>()),
    descSets(clock)
{
    if (info.maxShadowMaps == 0)
    {
        throw std::invalid_argument("[In ShadowPool::ShadowPool]: Maximum number of shadows must"
                                    " be greater than 0!");
    }

    createDescriptors(info.maxShadowMaps);
    for (ui32 i = 0; i < clock.getFrameCount(); i++) {
        writeDescriptors(i);
    }
}

void trc::ShadowPool::update()
{
    updateMatrixBuffer();
}

auto trc::ShadowPool::allocateShadow(const ShadowCreateInfo& info) -> ShadowMap
{
    const ui32 id = shadowIdPool.generate();
    if (id > kMaxShadowMaps)
    {
        shadowIdPool.free(id);
        throw std::out_of_range(
            "[In ShadowPool::allocateShadow]: Unable to allocate shadow - the maximum of "
            + std::to_string(kMaxShadowMaps) + " shadows is exceeded!"
        );
    }

    auto& shadow = shadows.emplace(
        id,
        std::make_unique<Shadow>(device, clock, id, info.shadowMapResolution)
    );

    // Update all descriptors
    for (ui32 i = 0; i < clock.getFrameCount(); i++) {
        writeDescriptors(i);
    }
    updateMatrixBuffer();

    return {
        .index      = id,
        .renderPass = shadow->renderPass,
        .camera     = shadow->camera
    };
}

auto trc::ShadowPool::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::ShadowPool::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    cmdBuf.bindDescriptorSets(bindPoint, pipelineLayout, setIndex, **descSets, {});
}

void trc::ShadowPool::updateMatrixBuffer()
{
    for (auto& shadow : shadows)
    {
        if (shadow == nullptr) continue;

        const auto& cam = shadow->camera;
        const mat4 viewProj = cam->getProjectionMatrix() * cam->getViewMatrix();
        shadowMatrixBufferMap[shadow->index] = viewProj;
    }
}

void trc::ShadowPool::createDescriptors(const ui32 maxShadowMaps)
{
    auto shaderStages = vk::ShaderStageFlagBits::eVertex
                        | vk::ShaderStageFlagBits::eFragment
                        | vk::ShaderStageFlagBits::eCompute
                        | rt::ALL_RAY_PIPELINE_STAGE_FLAGS;

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        // Shadow matrix buffer
        { 0, vk::DescriptorType::eStorageBuffer, 1, shaderStages },
        // Shadow maps
        { 1, vk::DescriptorType::eCombinedImageSampler, maxShadowMaps, shaderStages },
    };
    std::vector<vk::DescriptorBindingFlags> layoutFlags{
        {}, // shadow matrix buffer
        vk::DescriptorBindingFlagBits::eVariableDescriptorCount
        | vk::DescriptorBindingFlagBits::ePartiallyBound, // shadow map samplers
    };

    vk::StructureChain layoutChain{
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(layoutFlags)
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eCombinedImageSampler, maxShadowMaps },
    };
    descPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        clock.getFrameCount(), // max num sets
        poolSizes
    ));

    // Sets
    const ui32 numSets = clock.getFrameCount();
    const std::vector<ui32> max(numSets, maxShadowMaps);
    const std::vector<vk::DescriptorSetLayout> layouts(numSets, *descLayout);
    vk::StructureChain descSetAllocateChain{
        vk::DescriptorSetAllocateInfo(*descPool, layouts),
        vk::DescriptorSetVariableDescriptorCountAllocateInfo(numSets, max.data()),
    };
    descSets = {
        clock,
        device->allocateDescriptorSetsUnique(
            descSetAllocateChain.get<vk::DescriptorSetAllocateInfo>()
        )
    };

    // Write shadow matrix buffers only once
    descSets.foreach([this](auto& set) {
        vk::DescriptorBufferInfo bufferInfo(*shadowMatrixBuffer, 0, VK_WHOLE_SIZE);
        std::vector<vk::WriteDescriptorSet> writes = {
            { *set, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &bufferInfo },
        };

        device->updateDescriptorSets(writes, {});
    });
}

void trc::ShadowPool::writeDescriptors(ui32 frameIndex)
{
    auto descSet = *descSets.getAt(frameIndex);

    // Collect all shadow images
    std::vector<vk::DescriptorImageInfo> imageInfos;
    for (auto& shadow : shadows)
    {
        // Vector space is pre-allocated. Skip empty shadow slots.
        if (shadow == nullptr) continue;

        auto& pass = shadow->renderPass;
        auto imageView = pass->getShadowImageView(frameIndex);
        auto sampler = pass->getShadowImage(frameIndex).getDefaultSampler();
        imageInfos.emplace_back(sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    if (!imageInfos.empty())
    {
        vk::WriteDescriptorSet write(
            descSet, 1, 0, imageInfos.size(),
            vk::DescriptorType::eCombinedImageSampler,
            imageInfos.data()
        );

        device->updateDescriptorSets(write, {});
    }
}
