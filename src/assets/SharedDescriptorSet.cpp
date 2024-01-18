#include "trc/assets/SharedDescriptorSet.h"



/////////////////////////////
//  DescriptorSet Binding  //
/////////////////////////////

trc::SharedDescriptorSet::Binding::Binding(s_ptr<SharedDescriptorSet> _set, ui32 bindingIndex)
    :
    set(std::move(_set)),
    bindingIndex(bindingIndex)
{
    assert(set != nullptr);
}

auto trc::SharedDescriptorSet::Binding::getBindingIndex() const -> ui32
{
    return bindingIndex;
}

void trc::SharedDescriptorSet::Binding::update(ui32 arrayElem, vk::DescriptorBufferInfo buffer)
{
    assert(set != nullptr);
    set->update(bindingIndex, arrayElem, buffer);
}

void trc::SharedDescriptorSet::Binding::update(
    ui32 firstArrayElem,
    const vk::ArrayProxy<const vk::DescriptorBufferInfo>& buffers)
{
    assert(set != nullptr);
    set->update(bindingIndex, firstArrayElem, buffers);
}

void trc::SharedDescriptorSet::Binding::update(ui32 arrayElem, vk::DescriptorImageInfo image)
{
    assert(set != nullptr);
    set->update(bindingIndex, arrayElem, image);
}

void trc::SharedDescriptorSet::Binding::update(
    ui32 firstArrayElem,
    const vk::ArrayProxy<const vk::DescriptorImageInfo>& images)
{
    assert(set != nullptr);
    set->update(bindingIndex, firstArrayElem, images);
}

void trc::SharedDescriptorSet::Binding::update(ui32 arrayElem, vk::BufferView view)
{
    assert(set != nullptr);
    set->update(bindingIndex, arrayElem, view);
}

void trc::SharedDescriptorSet::Binding::update(
    ui32 firstArrayElem,
    const vk::ArrayProxy<const vk::BufferView>& views)
{
    assert(set != nullptr);
    set->update(bindingIndex, firstArrayElem, views);
}



/////////////////////////////
//  DescriptorSet Builder  //
/////////////////////////////


trc::SharedDescriptorSet::Builder::Builder()
    :
    set(new SharedDescriptorSet)
{
}

void trc::SharedDescriptorSet::Builder::addLayoutFlag(vk::DescriptorSetLayoutCreateFlags flags)
{
    layoutFlags |= flags;
}

void trc::SharedDescriptorSet::Builder::addPoolFlag(vk::DescriptorPoolCreateFlags flags)
{
    poolFlags |= flags;
}

auto trc::SharedDescriptorSet::Builder::addBinding(
    vk::DescriptorType type,
    ui32 count,
    vk::ShaderStageFlags stages,
    vk::DescriptorBindingFlags flags) -> Binding
{
    const ui32 index = static_cast<ui32>(bindings.size());
    bindings.emplace_back(index, type, count, stages);
    bindingFlags.emplace_back(flags);

    return { set, index };
}

auto trc::SharedDescriptorSet::Builder::build(const Device& device)
    -> s_ptr<SharedDescriptorSet>
{
    if (set == nullptr)
    {
        throw std::runtime_error(
            "[In SharedDescriptorSet::Builder::build]: `build` was called multiple times."
            " The `build` method may only be called once on any builder object!");
    }

    set->build(device, *this);
    return std::move(set);
}



/////////////////////
//  DescriptorSet  //
/////////////////////

void trc::SharedDescriptorSet::build(const Device& device, const Builder& builder)
{
    const ui32 numSets = 1;
    bindings = builder.bindings;

    // Create descriptor layout
    // VK_EXT_descriptor_indexing is included in 1.2
    vk::StructureChain chain{
        vk::DescriptorSetLayoutCreateInfo(builder.layoutFlags, builder.bindings),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(builder.bindingFlags)
    };

    layout = device->createDescriptorSetLayoutUnique(chain.get());

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (const auto& binding : builder.bindings) {
        poolSizes.emplace_back(binding.descriptorType, binding.descriptorCount);
    }

    pool = device->createDescriptorPoolUnique({
        builder.poolFlags | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        numSets, poolSizes
    });

    // Allocate descriptor set
    std::vector<vk::DescriptorSetLayout> layouts(numSets, *layout);
    auto sets = device->allocateDescriptorSetsUnique({ *pool, layouts });
    set = std::move(sets.at(0));

    provider->setDescriptorSet(*set);

    // Update descriptor sets
    // It is possible to specify descriptor updates during building.
    // Consume these updates now.
    for (auto& write : writes) {
        write.setDstSet(*set);
    }
    update(device);
}

auto trc::SharedDescriptorSet::build() -> Builder
{
    return Builder{};
}

auto trc::SharedDescriptorSet::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *layout;
}

auto trc::SharedDescriptorSet::getProvider() const -> s_ptr<const DescriptorProviderInterface>
{
    assert(provider != nullptr);
    return provider;
}

void trc::SharedDescriptorSet::update(const Device& device)
{
    std::scoped_lock lock(descriptorUpdateLock);
    if (!writes.empty())
    {
        device->updateDescriptorSets(writes, {});
        writes.clear();
        updateStructs.clear();
    }
}

void trc::SharedDescriptorSet::update(
    ui32 binding,
    ui32 firstArrayElem,
    const vk::ArrayProxy<const vk::DescriptorBufferInfo>& buffers)
{
    std::scoped_lock lock(descriptorUpdateLock);

    auto& info = updateStructs.emplace_back(buffers);
    writes.push_back(
        vk::WriteDescriptorSet(
            *set, binding, firstArrayElem,
            bindings.at(binding).descriptorType,
            {}, info.bufferInfos, {}
        )
    );
}

void trc::SharedDescriptorSet::update(
    ui32 binding,
    ui32 firstArrayElem,
    const vk::ArrayProxy<const vk::DescriptorImageInfo>& images)
{
    std::scoped_lock lock(descriptorUpdateLock);

    auto& info = updateStructs.emplace_back(images);
    writes.push_back(
        vk::WriteDescriptorSet(
            *set, binding, firstArrayElem,
            bindings.at(binding).descriptorType,
            info.imageInfos, {}, {}
        )
    );
}

void trc::SharedDescriptorSet::update(
    ui32 binding,
    ui32 firstArrayElem,
    const vk::ArrayProxy<const vk::BufferView>& bufferViews)
{
    std::scoped_lock lock(descriptorUpdateLock);

    auto& info = updateStructs.emplace_back(bufferViews);
    writes.push_back(
        vk::WriteDescriptorSet(
            *set, binding, firstArrayElem,
            bindings.at(binding).descriptorType,
            {}, {}, info.bufferViews
        )
    );
}
