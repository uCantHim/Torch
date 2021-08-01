#include "ShadowDescriptor.h"

#include <vkb/ImageUtils.h>

#include "core/Window.h"



trc::ShadowDescriptor::ShadowDescriptor(const Window& window)
    :
    window(window),
    descSets(window.getSwapchain()),
    provider(*descLayout, { window.getSwapchain() })
{
    const ui32 maxShadowMaps = LightRegistry::MAX_SHADOW_MAPS;

    createDescriptors(window, maxShadowMaps);
}

void trc::ShadowDescriptor::update(const LightRegistry& lightRegistry)
{
    writeDescriptors(lightRegistry);
}

auto trc::ShadowDescriptor::getProvider() const noexcept -> const DescriptorProviderInterface&
{
    return provider;
}

auto trc::ShadowDescriptor::getDescLayout() const noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::ShadowDescriptor::createDescriptors(
    const Window& window,
    const ui32 maxShadowMaps)
{
    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        // Shadow matrix buffer
        { 0, vk::DescriptorType::eStorageBuffer, 1,
          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
        // Shadow maps
        { 1, vk::DescriptorType::eCombinedImageSampler, LightRegistry::MAX_SHADOW_MAPS,
          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
    };
    std::vector<vk::DescriptorBindingFlags> layoutFlags{
        {}, // shadow matrix buffer
        vk::DescriptorBindingFlagBits::eVariableDescriptorCount, // shadow map samplers
    };

    vk::StructureChain layoutChain{
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(layoutFlags)
    };
    descLayout = window.getDevice()->createDescriptorSetLayoutUnique(
        layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eCombinedImageSampler, maxShadowMaps },
    };
    descPool = window.getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        window.getSwapchain().getFrameCount(), // max num sets
        poolSizes
    ));

    // Sets
    descSets = { window.getSwapchain(), [&](ui32)
    {
        vk::StructureChain descSetAllocateChain{
            vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout),
            vk::DescriptorSetVariableDescriptorCountAllocateInfo(1, &maxShadowMaps)
        };
        auto set = std::move(window.getDevice()->allocateDescriptorSetsUnique(
            descSetAllocateChain.get<vk::DescriptorSetAllocateInfo>()
        )[0]);

        return set;
    }};

    provider.setDescriptorSetLayout(*descLayout);
    provider.setDescriptorSet({
        window.getSwapchain(),
        [this](ui32 i) { return *descSets.getAt(i); }
    });
}

void trc::ShadowDescriptor::writeDescriptors(const LightRegistry& lightRegistry)
{
    if (lightRegistry.shadows.empty()) {
        return;
    }

    // Always update the shadow matrix buffer
    vk::DescriptorBufferInfo shadowMatrixBuffer(
        lightRegistry.getShadowMatrixBuffer(),
        0, VK_WHOLE_SIZE
    );
    std::vector<vk::WriteDescriptorSet> writes = {
        { **descSets, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &shadowMatrixBuffer },
    };

    // Collect image update infos
    std::vector<std::vector<vk::DescriptorImageInfo>> imageInfos;
    for (ui32 i = 0; i < window.getSwapchain().getFrameCount(); i++)
    {
        auto descSet = *descSets.getAt(i);
        auto& infos = imageInfos.emplace_back();

        for (auto& [light, shadow] : lightRegistry.shadows)
        {
            // Update shadow index on the light
            light->firstShadowIndex = infos.size();

            // TODO: Get the shadow images from another source than the
            // removed render passes now
            //
            //for (auto shadowPass : shadow.shadowPasses)
            //{
            //    auto imageView = shadowPass->getDepthImageView(i);
            //    auto sampler = shadowPass->getDepthImage(i).getDefaultSampler();
            //    shadowPass->setShadowMatrixIndex(infos.size());
            //    infos.emplace_back(sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
            //}
        }

        // Might be empty if hasShadow flags have been manually set to false
        if (!infos.empty())
        {
            writes.emplace_back(vk::WriteDescriptorSet(
                descSet, 1, 0, infos.size(),
                vk::DescriptorType::eCombinedImageSampler,
                infos.data()
            ));
        }
    }

    window.getDevice()->updateDescriptorSets(writes, {});
}
