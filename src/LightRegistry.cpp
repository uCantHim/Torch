#include "LightRegistry.h"

#include "utils/Util.h"
#include "PipelineDefinitions.h"



auto trc::LightRegistry::ShadowInfo::getNode() noexcept -> Node&
{
    return parentNode;
}

void trc::LightRegistry::ShadowInfo::setProjectionMatrix(mat4 proj) noexcept
{
    for (auto& camera : shadowCameras) {
        camera.setProjectionMatrix(proj);
    }
}



vkb::StaticInit trc::ShadowDescriptor::_init{
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
            vk::DescriptorSetLayoutCreateInfo({}, layoutBindings),
            vk::DescriptorSetLayoutBindingFlagsCreateInfo(layoutFlags)
        };
        descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
            layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
        );

        // Dummy resources in case the sampler descriptor count is specified as 0
        dummyShadowImage = []() {
            auto img =  vkb::makeImage2D(vec4(0.0f));
            img.changeLayout(vkb::getDevice(), vk::ImageLayout::eShaderReadOnlyOptimal);
            return img;
        }();
        dummyImageView = dummyShadowImage.createView(vk::ImageViewType::e2D,
                                                     vk::Format::eR8G8B8A8Unorm);
    },
    []() {
        descLayout.reset();
        dummyShadowImage = {};
        dummyImageView.reset();
    }
};

trc::ShadowDescriptor::ShadowDescriptor(const LightRegistry& lightRegistry, ui32 numShadowMaps)
{
    createDescriptors(lightRegistry, numShadowMaps);
}

auto trc::ShadowDescriptor::getProvider() const noexcept -> const DescriptorProviderInterface&
{
    return provider;
}

auto trc::ShadowDescriptor::getDescLayout() noexcept -> vk::DescriptorSetLayout
{
    return *descLayout;
}

void trc::ShadowDescriptor::createDescriptors(
    const LightRegistry& lightRegistry,
    const ui32 numShadowMaps)
{
    const ui32 actualNumShadowMaps = glm::max(numShadowMaps, 1u); // Can't have empty descriptors

    descPool = vkb::getDevice()->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        vkb::getSwapchain().getFrameCount(), // max num sets
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 1 },
            { vk::DescriptorType::eCombinedImageSampler, actualNumShadowMaps },
        }
    ));

    descSets = { [&](ui32)
    {
        vk::StructureChain descSetAllocateChain{
            vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout),
            vk::DescriptorSetVariableDescriptorCountAllocateInfo(1, &actualNumShadowMaps)
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

        // Write dummy value to descriptor if count is 0
        if (numShadowMaps == 0)
        {
            vk::DescriptorImageInfo imageInfo(
                dummyShadowImage.getDefaultSampler(),
                *dummyImageView,
                vk::ImageLayout::eShaderReadOnlyOptimal
            );
            vkb::getDevice()->updateDescriptorSets(vk::WriteDescriptorSet{
                *set, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo
            }, {});
        }

        return set;
    }};

    provider.setDescriptorSet({ [this](ui32 i) { return *descSets.getAt(i); } });
}



//////////////////////////////
//      Light registry      //
//////////////////////////////

trc::LightRegistry::LightRegistry(const ui32 maxLights)
    :
    maxLights(maxLights),
    maxShadowMaps(glm::min(maxLights * 4, ShadowDescriptor::MAX_SHADOW_MAPS)),
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
    nextFreeShadowPassIndex(internal::RenderPasses::eShadowPassesBegin),
    shadowDescriptor(new ShadowDescriptor(*this, 0))
{
}

void trc::LightRegistry::update()
{
    // TODO: Put these into a single buffer
    updateLightBuffer();
    updateShadowMatrixBuffer();
}

auto trc::LightRegistry::getDescriptor() const noexcept -> const ShadowDescriptor&
{
    return *shadowDescriptor;
}

auto trc::LightRegistry::getMaxLights() const noexcept -> ui32
{
    return maxLights;
}

auto trc::LightRegistry::addLight(Light& light) -> Light&
{
    auto& result = *lights.emplace_back(&light);
    // Don't have to update the descriptors here

    return result;
}

void trc::LightRegistry::removeLight(const Light& light)
{
    auto it = std::remove(lights.begin(), lights.end(), &light);
    if (it != lights.end()) {
        lights.erase(it);
    }

    updateShadowDescriptors();
}

auto trc::LightRegistry::enableShadow(
    Light& light,
    uvec2 shadowResolution
    ) -> ShadowInfo&
{
    if (light.type != Light::Type::eSunLight) {
        throw std::invalid_argument("Shadows are currently only supported for sun lights");
    }
    if (std::find(lights.begin(), lights.end(), &light) == lights.end()) {
        throw std::invalid_argument("Light does not exist in the light registry!");
    }
    if (shadows.find(&light) != shadows.end()) {
        throw std::invalid_argument("Shadows are already enabled for the light!");
    }

    auto [it, success] = shadows.try_emplace(&light);
    if (!success) {
        throw std::runtime_error("Unable to add light to the map in LightRegistry::enableShadow");
    }

    auto& newEntry = it->second;
    for (ui32 i = 0; i < getNumShadowMaps(light); i++)
    {
        auto& camera = newEntry.shadowCameras.emplace_back();
        newEntry.parentNode.attach(camera);
        // Use lookAt for sun lights
        if (light.type == Light::Type::eSunLight && length(light.direction) > 0.0f) {
            camera.lookAt(light.position, light.position + light.direction, vec3(0, 1, 0));
        }

        // Generate an index for the new shadow pass
        ui32 newIndex = 0;
        if (!freeShadowPassIndices.empty()) {
            newIndex = freeShadowPassIndices.back();
            freeShadowPassIndices.pop_back();
        }
        else {
            newIndex = nextFreeShadowPassIndex++;
        }

        // Create a new shadow pass
        RenderPassShadow* newShadowPass = newEntry.shadowPasses.emplace_back(
            &RenderPass::create<RenderPassShadow>(newIndex, shadowResolution)
        );
        shadowPasses.push_back(newShadowPass->id());
    }

    newEntry.parentNode.update();
    light.hasShadow = true;
    updateShadowDescriptors();

    return newEntry;
}

void trc::LightRegistry::disableShadow(Light& light)
{
    if (!light.hasShadow) return;
    auto it = shadows.find(&light);
    if (it == shadows.end()) return;

    for (RenderPassShadow* shadowPass : it->second.shadowPasses)
    {
        const RenderPassShadow::ID id = shadowPass->id();
        shadowPasses.erase(std::remove(shadowPasses.begin(), shadowPasses.end(), id));
        freeShadowPassIndices.push_back(id);
        RenderPass::destroy(id);
    }
    shadows.erase(it);

    updateShadowDescriptors();
}

auto trc::LightRegistry::getLightBuffer() const noexcept -> vk::Buffer
{
    return *lightBuffer;
}

auto trc::LightRegistry::getShadowMatrixBuffer() const noexcept -> vk::Buffer
{
    return *shadowMatrixBuffer;
}

auto trc::LightRegistry::getShadowRenderStage() const noexcept
    -> const std::vector<RenderPass::ID>&
{
    return shadowPasses;
}

void trc::LightRegistry::updateLightBuffer()
{
    assert(lights.size() <= maxLights);

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
    for (size_t i = 0; const auto& [light, shadow] : shadows)
    {
        for (const auto& camera : shadow.shadowCameras)
        {
            // Only increase counter for lights that have a shadow
            buf[i++] = camera.getProjectionMatrix() * camera.getViewMatrix();
        }
    }
    shadowMatrixBuffer.unmap();
}

void trc::LightRegistry::updateShadowDescriptors()
{
    /**
     * Just recreate the whole descriptor here. I have to recreate the
     * descriptor set because I can't have non-written (empty) samplers
     * in the shadow map descriptor.
     */
    shadowDescriptor.reset(new ShadowDescriptor(*this, [this]() -> ui32 {
        // Count required number of shadow maps
        ui32 result{ 0 };
        for (auto& [light, shadow] : shadows) { result += getNumShadowMaps(*light); }
        return result;
    }()));

    if (shadows.empty()) {
        return;
    }

    std::vector<std::vector<vk::DescriptorImageInfo>> imageInfos;
    std::vector<vk::WriteDescriptorSet> writes;

    for (ui32 i = 0; i < vkb::getSwapchain().getFrameCount(); i++)
    {
        auto descSet = *shadowDescriptor->descSets.getAt(i);
        auto& infos = imageInfos.emplace_back();

        for (auto& [light, shadow] : shadows)
        {
            // Update shadow index on the light
            light->firstShadowIndex = infos.size();

            for (auto shadowPass : shadow.shadowPasses)
            {
                auto imageView = shadowPass->getDepthImageView(i);
                auto sampler = shadowPass->getDepthImage(i).getDefaultSampler();
                shadowPass->setShadowMatrixIndex(infos.size());
                infos.emplace_back(sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
            }
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

    if (!writes.empty()) {
        vkb::getDevice()->updateDescriptorSets(writes, {});
    }

    // Ensure that no light tries to access an unset shadow map
    updateShadowMatrixBuffer();
}
