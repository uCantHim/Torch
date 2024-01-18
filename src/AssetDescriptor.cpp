#include "trc/AssetDescriptor.h"

#include "trc/ray_tracing/RayPipelineBuilder.h"  // For ALL_RAY_PIPELINE_STAGE_FLAGS



namespace trc
{

AssetDescriptor::AssetDescriptor(const Device& device, const AssetDescriptorCreateInfo& info)
{
    auto builder = SharedDescriptorSet::build();
    builder.addLayoutFlag(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
    builder.addPoolFlag(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);

    // Geometry related bindings
    bindings.emplace(AssetDescriptorBinding::eGeometryIndexBuffers, builder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        info.maxGeometries,
        rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
        vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    ));
    bindings.emplace(AssetDescriptorBinding::eGeometryVertexBuffers, builder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        info.maxGeometries,
        rt::ALL_RAY_PIPELINE_STAGE_FLAGS,
        vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    ));

    // Texture related bindings
    bindings.emplace(AssetDescriptorBinding::eTextureSamplers, builder.addBinding(
        vk::DescriptorType::eCombinedImageSampler,
        info.maxTextures,
        vk::ShaderStageFlagBits::eAll,
        vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    ));

    // Font related bindings
    bindings.emplace(AssetDescriptorBinding::eGlyphMapSamplers, builder.addBinding(
        vk::DescriptorType::eCombinedImageSampler,
        info.maxFonts,
        vk::ShaderStageFlagBits::eFragment,
        vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    ));

    // Animation related bindings
    bindings.emplace(AssetDescriptorBinding::eAnimationMetadata, builder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind
    ));
    bindings.emplace(AssetDescriptorBinding::eAnimationData, builder.addBinding(
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind
    ));

    descSet = builder.build(device);

    // Assert that the descriptors have been added in the correct order
    auto assertIndex = [this](AssetDescriptorBinding binding) {
        assert(bindings.at(binding).getBindingIndex() == getBindingIndex(binding));
    };
    assertIndex(AssetDescriptorBinding::eGeometryIndexBuffers);
    assertIndex(AssetDescriptorBinding::eGeometryVertexBuffers);
    assertIndex(AssetDescriptorBinding::eTextureSamplers);
    assertIndex(AssetDescriptorBinding::eGlyphMapSamplers);
    assertIndex(AssetDescriptorBinding::eAnimationMetadata);
    assertIndex(AssetDescriptorBinding::eAnimationData);
}

void AssetDescriptor::update(const Device& device)
{
    descSet->update(device);
}

auto AssetDescriptor::getBinding(AssetDescriptorBinding binding) -> Binding
{
    return bindings.at(binding);
}

auto AssetDescriptor::getBinding(AssetDescriptorBinding binding) const -> const Binding
{
    return bindings.at(binding);
}

auto AssetDescriptor::getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout
{
    return descSet->getDescriptorSetLayout();
}

void AssetDescriptor::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    descSet->getProvider()->bindDescriptorSet(cmdBuf, bindPoint, pipelineLayout, setIndex);
}

} // namespace trc
