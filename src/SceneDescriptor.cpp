#include "SceneDescriptor.h"

#include "core/Window.h"
#include "Scene.h"
#include "ray_tracing/RayPipelineBuilder.h"



trc::SceneDescriptor::SceneDescriptor(const Window& window)
    :
    window(window),
    device(window.getDevice()),
    lightBuffer(
        window.getDevice(),
        util::sizeof_pad_16_v<LightData> * 128,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    lightBufferMap(lightBuffer.map()),
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

    createDescriptors();
    writeDescriptors();
}

void trc::SceneDescriptor::update(const Scene& scene)
{
    updatePicking();

    const auto& lights = scene.getLights();

    // Resize light buffer if the current one is too small
    if (lights.getRequiredLightDataSize() > lightBuffer.size())
    {
        lightBuffer.unmap();

        // Create new buffer
        lightBuffer = vkb::Buffer(
            window.getDevice(),
            lights.getRequiredLightDataSize(),
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        );
        lightBufferMap = lightBuffer.map();

        // Update descriptor
        vk::DescriptorBufferInfo lightBufferInfo(*lightBuffer, 0, VK_WHOLE_SIZE);
        std::vector<vk::WriteDescriptorSet> writes = {
            { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBufferInfo },
        };
        device->updateDescriptorSets(writes, {});
    }

    // Update light data
    lights.writeLightData(lightBufferMap);
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

void trc::SceneDescriptor::createDescriptors()
{
    vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eFragment;
    if (window.getInstance().hasRayTracing()) {
        shaderStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
    }

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        // Light buffer
        { 0, vk::DescriptorType::eStorageBuffer, 1, shaderStages },
        // Picking buffer
        { 1, vk::DescriptorType::eStorageBuffer, 1, shaderStages },
    };
    std::vector<vk::DescriptorBindingFlags> flags{
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        {},  // No flags for the dynamic storage buffer
    };

    vk::StructureChain chain{
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, layoutBindings
        ),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(flags),
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        chain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    // Pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eStorageBuffer, 1 },
    };
    descPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        1, poolSizes
    ));

    // Sets
    descSet = std::move(device->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);
}

void trc::SceneDescriptor::writeDescriptors()
{
    vk::DescriptorBufferInfo pickingBufferInfo(*pickingBuffer, 0, PICKING_BUFFER_SECTION_SIZE);
    vk::DescriptorBufferInfo lightBufferInfo(*lightBuffer, 0, VK_WHOLE_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &pickingBufferInfo },
        { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBufferInfo },
    };
    device->updateDescriptorSets(writes, {});
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
        {}
        //frame * PICKING_BUFFER_SECTION_SIZE // dynamic offset
    );
}
