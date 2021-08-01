#include "SceneDescriptor.h"

#include "core/Window.h"
#include "Scene.h"



trc::SceneDescriptor::SceneDescriptor(const Window& window)
    :
    window(window),
    pickingBuffer(
        window.getDevice(),
        PICKING_BUFFER_SECTION_SIZE * window.getSwapchain().getFrameCount(),
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
    auto cmdBuf = window.getDevice().createTransferCommandBuffer();
    cmdBuf->begin(vk::CommandBufferBeginInfo());
    cmdBuf->fillBuffer(*pickingBuffer, 0, VK_WHOLE_SIZE, 0);
    cmdBuf->end();
    window.getDevice().executeTransferCommandBufferSyncronously(*cmdBuf);

    createDescriptors(window.getInstance());
}

void trc::SceneDescriptor::update(const Scene& scene)
{
    updatePicking();
    writeDescriptors(window.getInstance(), scene);
}

void trc::SceneDescriptor::updatePicking()
{
    /**
     * Only read the buffer every nth frame, where n is the number of
     * images in the swapchain.
     *
     * Coherency issues. This seems to be the best solution.
     *
     * I'm triggered. I use the eHostCoherent flag but see if Nvidia cares.
     */

    if (window.getSwapchain().getCurrentFrame() != 0) return;

    // Read data of the previous frame
    ui32* buf = reinterpret_cast<ui32*>(pickingBuffer.map(
        (window.getSwapchain().getFrameCount() - 1) * PICKING_BUFFER_SECTION_SIZE
    ));

    const ui32 newPicked = buf[0];

    // Reset buffer to default values
    buf[0] = 0u;
    buf[1] = 0u;
    reinterpret_cast<float*>(buf)[2] = 1.0f;

    pickingBuffer.unmap();

    // An object is being picked
    if (newPicked != NO_PICKABLE)
    {
        if (newPicked != currentlyPicked)
        {
            if (currentlyPicked != NO_PICKABLE)
            {
                PickableRegistry::getPickable(currentlyPicked).onUnpick();
                currentlyPicked = NO_PICKABLE;
            }
            PickableRegistry::getPickable(newPicked).onPick();
            currentlyPicked = newPicked;
        }
    }
    // No object is picked
    else
    {
        if (currentlyPicked != NO_PICKABLE)
        {
            PickableRegistry::getPickable(currentlyPicked).onUnpick();
            currentlyPicked = NO_PICKABLE;
        }
    }
}

auto trc::SceneDescriptor::getProvider() const noexcept -> const DescriptorProviderInterface&
{
    return provider;
}

auto trc::SceneDescriptor::getDescLayout() const noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::SceneDescriptor::createDescriptors(const Instance& instance)
{
    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        // Light buffer
        { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
        // Picking buffer
        { 1, vk::DescriptorType::eStorageBufferDynamic, 1, vk::ShaderStageFlagBits::eFragment },
    };
    descLayout = instance.getDevice()->createDescriptorSetLayoutUnique({ {}, layoutBindings });


    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eStorageBufferDynamic, 1 },
    };
    descPool = instance.getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes
    ));

    // Sets
    descSet = std::move(instance.getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);
}

void trc::SceneDescriptor::writeDescriptors(const Instance& instance, const Scene& scene)
{
    // Write descriptor set
    vk::DescriptorBufferInfo lightBufferInfo(scene.getLightBuffer(), 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo pickingBufferInfo(*pickingBuffer, 0, PICKING_BUFFER_SECTION_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBufferInfo },
        { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBufferDynamic, {}, &pickingBufferInfo },
    };
    instance.getDevice()->updateDescriptorSets(writes, {});
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
    return descriptor.getDescLayout();
}

void trc::SceneDescriptor::SceneDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    const ui32 frame = descriptor.window.getSwapchain().getCurrentFrame();

    cmdBuf.bindDescriptorSets(
        bindPoint,
        pipelineLayout,
        setIndex, *descriptor.descSet,
        frame * PICKING_BUFFER_SECTION_SIZE // dynamic offset
    );
}
