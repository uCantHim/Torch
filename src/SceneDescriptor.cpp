#include "trc/SceneDescriptor.h"

#include "trc/DescriptorSetUtils.h"
#include "trc/RasterSceneModule.h"
#include "trc/RaySceneModule.h"
#include "trc/core/SceneBase.h"
#include "trc/core/Window.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



trc::SceneDescriptor::SceneDescriptor(const Instance& instance)
    :
    instance(instance),
    device(instance.getDevice()),
    lightBuffer(
        instance.getDevice(),
        util::sizeof_pad_16_v<LightData> * 128,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    lightBufferMap(lightBuffer.map()),
    drawableDataBuf(
        instance.getDevice(),
        200 * sizeof(RaySceneModule::RayInstanceData),
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    drawableBufferMap(drawableDataBuf.map<std::byte*>())
{
    createDescriptors();
    writeDescriptors();
}

void trc::SceneDescriptor::update(const SceneBase& scene)
{
    updateRasterData(scene.getModule<RasterSceneModule>());
    updateRayData(scene.getModule<RaySceneModule>());
}

void trc::SceneDescriptor::updateRasterData(const RasterSceneModule& scene)
{
    const auto& lights = scene.getLights();

    // Resize light buffer if the current one is too small
    if (lights.getRequiredLightDataSize() > lightBuffer.size())
    {
        lightBuffer.unmap();

        // Create new buffer
        lightBuffer = Buffer(
            device,
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

void trc::SceneDescriptor::updateRayData(const RaySceneModule& scene)
{
    const size_t dataSize = scene.getMaxRayDeviceDataSize();
    if (dataSize > drawableDataBuf.size())
    {
        drawableDataBuf.unmap();
        drawableDataBuf = Buffer(
            instance.getDevice(),
            glm::max(dataSize, drawableDataBuf.size() * 2),
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible
        );
        drawableBufferMap = drawableDataBuf.map<std::byte*>();

        vk::DescriptorBufferInfo bufferInfo(*drawableDataBuf, 0, VK_WHOLE_SIZE);
        std::vector<vk::WriteDescriptorSet> writes = {
            { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &bufferInfo },
        };
        device->updateDescriptorSets(writes, {});
    }

    scene.writeRayDeviceData(drawableBufferMap, dataSize);
    drawableDataBuf.flush();
}

auto trc::SceneDescriptor::getProvider() const noexcept -> const DescriptorProviderInterface&
{
    return provider;
}

void trc::SceneDescriptor::createDescriptors()
{
    vk::ShaderStageFlags shaderStages = vk::ShaderStageFlagBits::eFragment
                                        | vk::ShaderStageFlagBits::eCompute;
    if (instance.hasRayTracing()) {
        shaderStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS;
    }

    // Layout
    descLayout = buildDescriptorSetLayout()
        .addFlag(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
        // Shadow data buffer
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, shaderStages,
                    vk::DescriptorBindingFlagBits::eUpdateAfterBind)
        // Ray tracing related drawable data buffer
        .addBinding(vk::DescriptorType::eStorageBuffer, 1, shaderStages,
                    vk::DescriptorBindingFlagBits::eUpdateAfterBind)
        .build(device);

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
    vk::DescriptorBufferInfo drawableBufferInfo(*drawableDataBuf, 0, VK_WHOLE_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBufferInfo },
        { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &drawableBufferInfo },
    };
    device->updateDescriptorSets(writes, {});
}



trc::SceneDescriptor::SceneDescriptorProvider::SceneDescriptorProvider(
    const SceneDescriptor& descriptor)
    : descriptor(descriptor)
{}

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
