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
    lightBufferMap(lightBuffer.map())
{
    createDescriptors();
    writeDescriptors();
}

void trc::SceneDescriptor::update(const Scene& scene)
{
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

auto trc::SceneDescriptor::getProvider() const noexcept -> const DescriptorProviderInterface&
{
    return provider;
}

void trc::SceneDescriptor::createDescriptors()
{
    vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eFragment
                                        | vk::ShaderStageFlagBits::eCompute;
    if (window.getInstance().hasRayTracing()) {
        shaderStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
    }

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        // Light buffer
        { 0, vk::DescriptorType::eStorageBuffer, 1, shaderStages },
    };
    std::vector<vk::DescriptorBindingFlags> flags{
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
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
    vk::DescriptorBufferInfo lightBufferInfo(*lightBuffer, 0, VK_WHOLE_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
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
    return *descriptor.descLayout;
}

void trc::SceneDescriptor::SceneDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    cmdBuf.bindDescriptorSets(
        bindPoint,
        pipelineLayout,
        setIndex, *descriptor.descSet,
        {}
    );
}
