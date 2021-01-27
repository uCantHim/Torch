#include "SceneDescriptor.h"

#include "Scene.h"



vkb::StaticInit trc::SceneDescriptor::_init{
    []() {
        // Create the static descriptor set layout
        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
            // Light buffer
            { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
            // Picking buffer
            { 1, vk::DescriptorType::eStorageBufferDynamic, 1, vk::ShaderStageFlagBits::eFragment },
        };
        descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique({ {}, layoutBindings });
    },
    []() {
        descLayout.reset();
    }
};



trc::SceneDescriptor::SceneDescriptor(const Scene& scene)
    :
    pickingBuffer(
        PICKING_BUFFER_SECTION_SIZE * vkb::getSwapchain().getFrameCount(),
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
    auto cmdBuf = vkb::getDevice().createTransferCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo());
    cmdBuf->fillBuffer(*pickingBuffer, 0, VK_WHOLE_SIZE, 0);
    cmdBuf->end();
    vkb::getDevice().executeTransferCommandBufferSyncronously(*cmdBuf);

    createDescriptors(scene);
}

auto trc::SceneDescriptor::updatePicking() -> Maybe<ui32>
{
    /**
     * Only read the buffer every nth frame, where n is the number of
     * images in the swapchain.
     *
     * Coherency issues. This seems to be the best solution.
     *
     * I'm triggered. I use the eHostCoherent flag but see if Nvidia cares.
     */
    if (vkb::getSwapchain().getCurrentFrame() != 0) return pickedObject;

    // Read data of the previous frame
    ui32* buf = reinterpret_cast<ui32*>(pickingBuffer.map(
        (vkb::getSwapchain().getFrameCount() - 1) * PICKING_BUFFER_SECTION_SIZE
    ));

    pickedObject = buf[0];

    // Reset buffer to default values
    buf[0] = 0u;
    buf[1] = 0u;
    reinterpret_cast<float*>(buf)[2] = 1.0f;

    pickingBuffer.unmap();

    if (pickedObject != NO_PICKABLE) {
        return pickedObject;
    }
    return {};
}

auto trc::SceneDescriptor::getProvider() const noexcept -> const DescriptorProviderInterface&
{
    return provider;
}

auto trc::SceneDescriptor::getDescLayout() noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::SceneDescriptor::createDescriptors(const Scene& scene)
{
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eStorageBufferDynamic, 1 },
    };
    descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes
    ));

    descSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    // Write descriptor set
    vk::DescriptorBufferInfo lightBufferInfo(scene.getLightBuffer(), 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo pickingBufferInfo(*pickingBuffer, 0, VK_WHOLE_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBufferInfo },
        { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBufferDynamic, {}, &pickingBufferInfo },
    };
    vkb::getDevice()->updateDescriptorSets(writes, {});
}



trc::SceneDescriptor::SceneDescriptorProvider::SceneDescriptorProvider(
    const SceneDescriptor& descriptor)
    : descriptor(descriptor)
{}

auto trc::SceneDescriptor::SceneDescriptorProvider::getDescriptorSet() const noexcept
    -> vk::DescriptorSet
{
    return *descriptor.descSet;
}

auto trc::SceneDescriptor::SceneDescriptorProvider::getDescriptorSetLayout() const noexcept
    -> vk::DescriptorSetLayout
{
    return SceneDescriptor::getDescLayout();
}

void trc::SceneDescriptor::SceneDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    ui32 pickingBufferOffset = vkb::getSwapchain().getCurrentFrame() * PICKING_BUFFER_SECTION_SIZE;
    cmdBuf.bindDescriptorSets(
        bindPoint,
        pipelineLayout,
        setIndex, *descriptor.descSet,
        pickingBufferOffset // dynamic offset
    );
}
