#include "text/FontDataStorage.h"

#include "core/Instance.h"



trc::FontDataStorage::FontDataStorage(const Instance& instance)
    :
    instance(instance),
    memoryPool(instance.getDevice(), 50000000)
{
    // Create descriptor set layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        { 0, vk::DescriptorType::eCombinedImageSampler, 1,
          vk::ShaderStageFlagBits::eFragment }
    };
    descLayout = instance.getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
    };
    descPool = instance.getDevice()->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes
    });
}

auto trc::FontDataStorage::allocateGlyphMap() -> std::pair<GlyphMap*, DescriptorProvider>
{
    auto& map = glyphMaps.emplace_back(instance.getDevice(), memoryPool.makeAllocator());
    auto& set = glyphMapDescSets.emplace_back(makeDescSet(map));

    return { &map, DescriptorProvider{ *descLayout, *set.set } };
}

auto trc::FontDataStorage::makeFont(const fs::path& filePath, ui32 fontSize) -> Font
{
    return Font(*this, filePath, fontSize);
}

auto trc::FontDataStorage::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *descLayout;
}

auto trc::FontDataStorage::makeDescSet(GlyphMap& glyphMap) -> GlyphMapDescriptorSet
{
    auto imageView = glyphMap.getGlyphImage().createView();
    auto set = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    vk::DescriptorImageInfo glyphImageInfo(
        glyphMap.getGlyphImage().getDefaultSampler(),
        *imageView,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );
    std::vector<vk::WriteDescriptorSet> writes{
        { *set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &glyphImageInfo },
    };

    instance.getDevice()->updateDescriptorSets(writes, {});

    return {
        std::move(imageView),
        std::move(set)
    };
}
