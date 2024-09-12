#include "trc/ShadowPool.h"

#include <ranges>

#include <trc_util/Assert.h>

#include "trc/DescriptorSetUtils.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



namespace trc
{

ShadowMap::ShadowMap(const Device& device, ui32 index, const ShadowMapCreateInfo& info)
    :
    index(index),
    renderPass(std::make_shared<RenderPassShadow>(
        device,
        ShadowPassCreateInfo{ index, info.shadowMapResolution }
    )),
    camera(info.camera)
{
    assert_arg(info.camera != nullptr);
}

auto ShadowMap::getCamera() -> Camera&
{
    return *camera;
}

auto ShadowMap::getCamera() const -> const Camera&
{
    return *camera;
}

auto ShadowMap::getRenderPass() -> RenderPassShadow&
{
    return *renderPass;
}

auto ShadowMap::getRenderPass() const -> const RenderPassShadow&
{
    return *renderPass;
}

auto ShadowMap::getImage() const -> vk::Image
{
    return *renderPass->getShadowImage();
}

auto ShadowMap::getImageView() const -> vk::ImageView
{
    return renderPass->getShadowImageView();
}

auto ShadowMap::getShadowMatrixIndex() const -> ui32
{
    return index;
}



ShadowPool::ShadowPool(
    const Device& device,
    const ShadowPoolCreateInfo& info,
    const DeviceMemoryAllocator& alloc)
    :
    device(device),
    kMaxShadowMaps(info.maxShadowMaps),
    // Must be a storage buffer rather than a uniform buffer because it has
    // a dynamically sized array in GLSL.
    shadowMatrixBuffer(
        device, sizeof(mat4) * info.maxShadowMaps,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal
        | vk::MemoryPropertyFlagBits::eHostCoherent,
        alloc
    ),
    shadowMatrixBufferMap(shadowMatrixBuffer.map<mat4*>())
{
    if (info.maxShadowMaps == 0)
    {
        throw std::invalid_argument("[In ShadowPool::ShadowPool]: Maximum number of shadows must"
                                    " be greater than 0!");
    }
}

void ShadowPool::update()
{
    updateMatrixBuffer();
}

auto ShadowPool::allocateShadow(ui32 index, const ShadowMapCreateInfo& info) -> s_ptr<ShadowMap>
{
    if (index >= kMaxShadowMaps)
    {
        throw std::invalid_argument(
            "[In ShadowPool::allocateShadow]: Invalid shadow map index: Must not exceed"
            " the maximum of " + std::to_string(kMaxShadowMaps) + " shadow maps!"
        );
    }

    updateMatrixBuffer();

    return shadowMaps.emplace(device, index, info);
}

auto ShadowPool::getMatrixBuffer() const -> const Buffer&
{
    return shadowMatrixBuffer;
}

auto ShadowPool::getShadows() const -> std::generator<const ShadowMap&>
{
    return shadowMaps.iterValidElements();
};

void ShadowPool::updateMatrixBuffer()
{
    for (auto& shadow : shadowMaps.iterValidElementsWithCleanup())
    {
        const auto& cam = shadow.getCamera();
        const mat4 viewProj = cam.getProjectionMatrix() * cam.getViewMatrix();
        shadowMatrixBufferMap[shadow.getShadowMatrixIndex()] = viewProj;
    }

    shadowMatrixBuffer.flush();
}



ShadowDescriptor::ShadowDescriptor(
    const Device& device,
    ui32 maxShadowMaps,
    ui32 maxDescriptorSets)
{
    constexpr auto shaderStages = vk::ShaderStageFlagBits::eVertex
                                | vk::ShaderStageFlagBits::eFragment
                                | vk::ShaderStageFlagBits::eCompute
                                | rt::ALL_RAY_PIPELINE_STAGE_FLAGS;

    auto builder = buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, shaderStages)
        .addBinding(vk::DescriptorType::eCombinedImageSampler, maxShadowMaps, shaderStages,
                    vk::DescriptorBindingFlagBits::ePartiallyBound);

    descLayout = builder.build(device);
    descPool = builder.buildPool(device, maxDescriptorSets,
                                 vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    device.setDebugName(*descLayout, "Shadow descriptor set layout");
    device.setDebugName(*descPool, "Shadow descriptor pool");
}

auto ShadowDescriptor::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *descLayout;
}

auto ShadowDescriptor::makeDescriptorSet(
    const Device& device,
    const ShadowPool& shadowPool)
    -> s_ptr<DescriptorSet>
{
    auto descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, *descLayout })[0]);
    device.setDebugName(*descSet, "Shadow descriptor set");

    return std::make_shared<DescriptorSet>(device, std::move(descSet), shadowPool);
}



ShadowDescriptor::DescriptorSet::DescriptorSet(
    const Device& device,
    vk::UniqueDescriptorSet _set,
    const ShadowPool& pool)
    :
    descSet(std::move(_set))
{
    // Write shadow matrix buffer only once
    vk::DescriptorBufferInfo bufferInfo(*pool.getMatrixBuffer(), 0, VK_WHOLE_SIZE);
    std::vector<vk::WriteDescriptorSet> writes{
        { *descSet, 0, 0, vk::DescriptorType::eStorageBuffer, {}, bufferInfo },
    };
    device->updateDescriptorSets(writes, {});

    update(device, pool);
}

void ShadowDescriptor::DescriptorSet::update(
    const Device& device,
    const ShadowPool& shadowPool)
{
    // Collect all shadow images
    std::vector<std::pair<ui32, vk::DescriptorImageInfo>> imageInfos;
    for (const ShadowMap& shadow : shadowPool.getShadows())
    {
        const ui32 index = shadow.getShadowMatrixIndex();

        const auto& pass = shadow.getRenderPass();
        auto imageView = pass.getShadowImageView();
        auto sampler = pass.getShadowImage().getDefaultSampler();
        imageInfos.emplace_back(
            index,
            vk::DescriptorImageInfo{ sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal }
        );
    }

    // Update the image descriptor
    if (!imageInfos.empty())
    {
        auto imageWrites = imageInfos
            | std::views::transform([this](auto& pair) {
                const auto& [index, image] = pair;
                return vk::WriteDescriptorSet{
                    *descSet, 1, index, vk::DescriptorType::eCombinedImageSampler,
                    image,
                };
            })
            | std::ranges::to<std::vector>();

        device->updateDescriptorSets(imageWrites, {});
    }
}

void ShadowDescriptor::DescriptorSet::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    cmdBuf.bindDescriptorSets(bindPoint, pipelineLayout, setIndex, *descSet, {});
}

} // namespace trc
