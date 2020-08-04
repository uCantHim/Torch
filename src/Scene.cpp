#include "Scene.h"

#include <cstring>

#include "utils/Util.h"
#include "PickableRegistry.h"



trc::Scene::Scene()
    :
    lightBuffer(
        util::sizeof_pad_16_v<Light> * MAX_LIGHTS,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    pickingBuffer(
        sizeof(ui32) * 3,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
    auto buf = reinterpret_cast<ui32*>(pickingBuffer.map());
    buf[0] = 0u;
    buf[1] = 0u;
    reinterpret_cast<float*>(buf)[2] = 1.0f;
    pickingBuffer.unmap();

    updateLightBuffer();
}

auto trc::Scene::getRoot() noexcept -> Node&
{
    return root;
}

auto trc::Scene::getRoot() const noexcept -> const Node&
{
    return root;
}

void trc::Scene::updateTransforms()
{
    root.updateAsRoot();
    updateLightBuffer();
    updatePicking();
}

void trc::Scene::add(SceneRegisterable& object)
{
    object.attachToScene(*this);
}

void trc::Scene::addLight(Light& light)
{
    lights.push_back(&light);
}

void trc::Scene::removeLight(Light& light)
{
    lights.erase(std::find(lights.begin(), lights.end(), &light));
}

auto trc::Scene::getLightBuffer() const noexcept -> vk::Buffer
{
    return *lightBuffer;
}

auto trc::Scene::getPickingBuffer() const noexcept -> vk::Buffer
{
    return *pickingBuffer;
}

auto trc::Scene::getPickedObject() -> std::optional<Pickable*>
{
    if (currentlyPicked == 0) {
        return std::nullopt;
    }

    return &PickableRegistry::getPickable(currentlyPicked);
}

void trc::Scene::updateLightBuffer()
{
    assert(lights.size() <= MAX_LIGHTS);

    auto buf = lightBuffer.map();

    const ui32 numLights = lights.size();
    memcpy(buf, &numLights, sizeof(ui32));
    for (size_t offset = sizeof(vec4); const Light* light : lights)
    {
        memcpy(buf + offset, light, sizeof(Light));
        offset += util::sizeof_pad_16_v<Light>;
    }

    lightBuffer.unmap();
}

void trc::Scene::updatePicking()
{
    auto buf = reinterpret_cast<ui32*>(pickingBuffer.map());
    ui32 newPicked = buf[0];
    buf[0] = 0u;
    buf[1] = 0u;
    reinterpret_cast<float*>(buf)[2] = 1.0f;
    pickingBuffer.unmap();

    // Unpick previously picked object
    if (newPicked != currentlyPicked)
    {
        if (currentlyPicked != 0)
        {
            PickableRegistry::getPickable(currentlyPicked).onUnpick();
        }
        if (newPicked != 0)
        {
            PickableRegistry::getPickable(newPicked).onPick();
        }
        currentlyPicked = newPicked;
    }
}



auto trc::SceneDescriptor::getProvider() noexcept -> const DescriptorProviderInterface&
{
    static vkb::VulkanStaticInitialization<SceneDescriptor> _force_init;

    return provider;
}

void trc::SceneDescriptor::setActiveScene(const Scene& scene) noexcept
{
    vk::DescriptorBufferInfo lightBuffer(scene.getLightBuffer(), 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo pickingBuffer(scene.getPickingBuffer(), 0, VK_WHOLE_SIZE);

    std::vector<vk::WriteDescriptorSet> writes = {
        { *descSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBuffer },
        { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &pickingBuffer },
    };
    vkb::getDevice()->updateDescriptorSets(writes, {});
}

void trc::SceneDescriptor::vulkanStaticInit()
{
    descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        1,
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 1 }, // Light buffer
            { vk::DescriptorType::eStorageBuffer, 1 }, // Picking buffer
        }
    ));

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
        { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
    };
    std::vector<vk::DescriptorBindingFlags> layoutFlags{
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
    };
    vk::StructureChain layoutChain
    {
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, layoutBindings
        ),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(layoutFlags)
    };
    descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    descSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

    provider.setDescriptorSetLayout(*descLayout);
    provider.setDescriptorSet(*descSet);
}

void trc::SceneDescriptor::vulkanStaticDestroy()
{
    descSet.reset();
    descLayout.reset();
    descPool.reset();
}
