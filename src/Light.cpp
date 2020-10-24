#include "Light.h"

#include "utils/Util.h"



trc::LightNode::LightNode(Light& light)
    :
    light(&light),
    initialDirection(light.direction)
{
}

void trc::LightNode::applyTransformToLight()
{
    assert(light != nullptr);

    light->position = { getTranslation(), 1.0f };
    light->direction = getRotationAsMatrix() * initialDirection;
}



vkb::StaticInit trc::_ShadowDescriptor::_init{
    []() {
        // Create the static descriptor set layout
        std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
            // Shadow matrix buffer
            { 0, vk::DescriptorType::eStorageBuffer, 1,
              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
            // Shadow maps
            { 1, vk::DescriptorType::eCombinedImageSampler, MAX_SHADOW_MAPS,
              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
        };
        std::vector<vk::DescriptorBindingFlags> layoutFlags{
            {}, // shadow matrix buffer
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount, // shadow map samplers
        };

        vk::StructureChain layoutChain{
            vk::DescriptorSetLayoutCreateInfo(
                vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, layoutBindings
            ),
            vk::DescriptorSetLayoutBindingFlagsCreateInfo(layoutFlags)
        };
        descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
            layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
        );
    },
    []() {
        descLayout.reset();
    }
};

trc::_ShadowDescriptor::_ShadowDescriptor(const LightRegistry& lightRegistry, ui32 maxShadowMaps)
{
    createDescriptors(lightRegistry, maxShadowMaps);
}

auto trc::_ShadowDescriptor::getDescSet(ui32 imageIndex) const noexcept -> vk::DescriptorSet
{
    return *descSets.getAt(imageIndex);
}

auto trc::_ShadowDescriptor::getDescLayout() noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::_ShadowDescriptor::createDescriptors(
    const LightRegistry& lightRegistry,
    const ui32 maxShadowMaps)
{
    descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
            | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        vkb::getSwapchain().getFrameCount(), // max num sets
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 1 },
            { vk::DescriptorType::eCombinedImageSampler, maxShadowMaps },
        }
    ));

    descSets = { [&](ui32)
    {
        vk::StructureChain descSetAllocateChain{
            vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout),
            vk::DescriptorSetVariableDescriptorCountAllocateInfo(1, &maxShadowMaps)
        };
        auto set = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
            descSetAllocateChain.get<vk::DescriptorSetAllocateInfo>()
        )[0]);

        // Write descriptor set
        vk::DescriptorBufferInfo shadowMatrixBuffer(
            lightRegistry.getShadowMatrixBuffer(),
            0, VK_WHOLE_SIZE
        );

        std::vector<vk::WriteDescriptorSet> writes = {
            { *set, 0, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &shadowMatrixBuffer },
        };
        vkb::getDevice()->updateDescriptorSets(writes, {});

        return set;
    }};
}


trc::LightRegistry::LightRegistry(const ui32 maxLights)
    :
    maxLights(maxLights),
    maxShadowMaps(min(maxLights * 4, _ShadowDescriptor::MAX_SHADOW_MAPS)),
    lightBuffer(
        util::sizeof_pad_16_v<Light> * maxLights,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    shadowMatrixBuffer(
        sizeof(mat4) * maxShadowMaps,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    ),
    shadowDescriptor(*this, maxShadowMaps)
{
}

void trc::LightRegistry::update()
{
    for (auto& [light, node] : lightNodes) {
        node->applyTransformToLight();
    }

    // TODO: Put these into a single buffer
    updateLightBuffer();
    updateShadowMatrixBuffer();
}

auto trc::LightRegistry::getDescriptor() const noexcept -> const _ShadowDescriptor&
{
    return shadowDescriptor;
}

auto trc::LightRegistry::getMaxLights() const noexcept -> ui32
{
    return maxLights;
}

auto trc::LightRegistry::addLight(Light& light) -> Light&
{
    auto& result = *lights.emplace_back(&light);
    updateShadowDescriptors();

    return result;
}

void trc::LightRegistry::removeLight(const Light& light)
{
    removeLightNode(light);
    auto it = std::remove(lights.begin(), lights.end(), &light);
    if (it != lights.end()) {
        lights.erase(it);
    }

    updateShadowDescriptors();
}

auto trc::LightRegistry::createLightNode(Light& light) -> LightNode&
{
    return *lightNodes.emplace_back(
        &light,
        std::make_unique<LightNode>(light)
    ).second;
}

void trc::LightRegistry::removeLightNode(const LightNode& node)
{
    auto it = std::find_if(
        lightNodes.begin(), lightNodes.end(),
        [&node](const auto& pair) { return pair.second.get() == &node; }
    );

    if (it != lightNodes.end()) {
        lightNodes.erase(it);
    }
}

void trc::LightRegistry::removeLightNode(const Light& light)
{
    auto it = std::find_if(
        lightNodes.begin(), lightNodes.end(),
        [&light](const auto& pair) { return pair.first == &light; }
    );

    if (it != lightNodes.end()) {
        lightNodes.erase(it);
    }
}

auto trc::LightRegistry::getLightBuffer() const noexcept -> vk::Buffer
{
    return *lightBuffer;
}

auto trc::LightRegistry::getShadowMatrixBuffer() const noexcept -> vk::Buffer
{
    return *shadowMatrixBuffer;
}

void trc::LightRegistry::updateLightBuffer()
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

void trc::LightRegistry::updateShadowMatrixBuffer()
{
    if (shadows.empty()) {
        return;
    }

    mat4* buf = reinterpret_cast<mat4*>(shadowMatrixBuffer.map());
    for (size_t i = 0; const Light* light : lights)
    {
        if (light->hasShadow)
        {
            auto it = shadows.find(light);
            assert(it != shadows.end());

            const auto& camera = it->second.shadowCamera;
            buf[i++] = camera.getProjectionMatrix() * camera.getViewMatrix();
        }
    }
}

void trc::LightRegistry::updateShadowDescriptors()
{
    if (shadows.empty()) {
        return;
    }

    std::vector<std::vector<vk::DescriptorImageInfo>> imageInfos;
    std::vector<vk::WriteDescriptorSet> writes;

    for (ui32 i = 0; i < vkb::getSwapchain().getFrameCount(); i++)
    {
        auto descSet = shadowDescriptor.getDescSet(i);
        auto& infos = imageInfos.emplace_back();

        for (Light* light : lights)
        {
            if (!light->hasShadow) continue;
            auto it = shadows.find(light);
            if (it == shadows.end()) continue;

            // Update shadow index on the light
            light->firstShadowIndex = infos.size();

            const auto& shadowPass = it->second.shadowPass;
            auto imageView = shadowPass.getDepthImageView(i);
            auto sampler = shadowPass.getDepthImage(i).getDefaultSampler();
            infos.emplace_back(sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        writes.emplace_back(vk::WriteDescriptorSet(
            descSet, 1, 0, infos.size(),
            vk::DescriptorType::eCombinedImageSampler,
            infos.data()
        ));
    }

    vkb::getDevice()->updateDescriptorSets(writes, {});

    // Ensure that no light tries to access an unset shadow map
    updateShadowMatrixBuffer();
}



auto trc::makeSunLight(vec3 color, vec3 direction, float ambientPercent) -> Light
{
    return {
        .color                = vec4(color, 1.0f),
        .position             = vec4(0.0f),
        .direction            = vec4(direction, 0.0f),
        .ambientPercentage    = ambientPercent,
        .attenuationLinear    = 0.0f,
        .attenuationQuadratic = 0.0f,
        .type                 = Light::Type::eSunLight
    };
}

auto trc::makePointLight(vec3 color,
                         vec3 position,
                         float attLinear,
                         float attQuadratic) -> Light
{
    return {
        .color                = vec4(color, 1.0f),
        .position             = vec4(position, 1.0f),
        .direction            = vec4(0.0f),
        .ambientPercentage    = 0.0f,
        .attenuationLinear    = attLinear,
        .attenuationQuadratic = attQuadratic,
        .type                 = Light::Type::ePointLight
    };
}

auto trc::makeAmbientLight(vec3 color) -> Light
{
    return {
        .color                = vec4(color, 1.0f),
        .position             = vec4(0.0f),
        .direction            = vec4(0.0f),
        .ambientPercentage    = 1.0f,
        .attenuationLinear    = 0.0f,
        .attenuationQuadratic = 0.0f,
        .type                 = Light::Type::eAmbientLight
    };
}
