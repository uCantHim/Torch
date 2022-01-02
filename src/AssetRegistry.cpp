#include "AssetRegistry.h"

#include <vkb/ImageUtils.h>

#include "core/Instance.h"
#include "ray_tracing/RayPipelineBuilder.h"
#include "util/TriangleCacheOptimizer.h"



trc::AssetRegistry::AssetRegistry(
    const Instance& instance,
    const AssetRegistryCreateInfo& info)
    :
    instance(instance),
    device(instance.getDevice()),
    memoryPool([&] {
        if (info.enableRayTracing)
        {
            return vkb::MemoryPool(
                instance.getDevice(),
                MEMORY_POOL_CHUNK_SIZE,
                vk::MemoryAllocateFlagBits::eDeviceAddress
            );
        }
        else {
            return vkb::MemoryPool(instance.getDevice(), MEMORY_POOL_CHUNK_SIZE);
        }
    }()),
    config(addDefaultValues(info)),
    materialBuffer(
        instance.getDevice(),
        MATERIAL_BUFFER_DEFAULT_SIZE,  // Default material buffer size
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    fontData(instance),
    animationStorage(instance)
{
    createDescriptors();

    // Add default assets
    add(Material{ .performLighting=false });
    updateMaterials();
    add(vkb::makeSinglePixelImage(device, vec4(1.0f)));

    writeDescriptors();
}

auto trc::AssetRegistry::add(const GeometryData& data, std::optional<RigData> rigData)
    -> GeometryID
{
    GeometryID key(nextGeometryIndex++, *this);

    addToMap(geometries, key,
        GeometryStorage{
            .indexBuf = {
                device,
                util::optimizeTriangleOrderingForsyth(data.indices),
                config.geometryBufferUsage | vk::BufferUsageFlagBits::eIndexBuffer,
                memoryPool.makeAllocator()
            },
            .vertexBuf = {
                device,
                data.vertices,
                config.geometryBufferUsage | vk::BufferUsageFlagBits::eVertexBuffer,
                memoryPool.makeAllocator()
            },
            .numIndices = static_cast<ui32>(data.indices.size()),
            .numVertices = static_cast<ui32>(data.vertices.size()),

            .rig = rigData.has_value()
                ? std::optional<Rig>(Rig(rigData.value(), animationStorage))
                : std::nullopt,
        }
    );

    if (config.enableRayTracing)
    {
        writeDescriptors();
    }

    return key;
}

auto trc::AssetRegistry::add(Material mat) -> MaterialID
{
    MaterialID key(nextMaterialIndex++, *this);
    addToMap(materials, key, mat);

    updateMaterials();

    return key;
}

auto trc::AssetRegistry::add(vkb::Image image) -> TextureID
{
    TextureID key(nextImageIndex++, *this);

    auto view = image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm);
    addToMap(textures, key,
        TextureStorage{
            .image = std::move(image),
            .imageView = std::move(view),
        }
    );

    writeDescriptors();

    return key;
}

auto trc::AssetRegistry::get(GeometryID key) -> Geometry
{
    return getFromMap(geometries, key);
}

auto trc::AssetRegistry::get(MaterialID key) -> Material&
{
    return getFromMap(materials, key);
}

auto trc::AssetRegistry::get(TextureID key) -> Texture
{
    return getFromMap(textures, key);
}

auto trc::AssetRegistry::getFonts() -> FontDataStorage&
{
    return fontData;
}

auto trc::AssetRegistry::getFonts() const -> const FontDataStorage&
{
    return fontData;
}

auto trc::AssetRegistry::getAnimations() -> AnimationDataStorage&
{
    return animationStorage;
}

auto trc::AssetRegistry::getAnimations() const -> const AnimationDataStorage&
{
    return animationStorage;
}

auto trc::AssetRegistry::getDescriptorSetProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return descriptorProvider;
}

void trc::AssetRegistry::updateMaterials()
{
    auto buf = reinterpret_cast<Material*>(materialBuffer.map());
    for (size_t i = 0; i < materials.size(); i++)
    {
        MaterialID::ID id(i);
        if (materials[id] != nullptr) {
            buf[i] = *materials[id];
        }
    }
    materialBuffer.unmap();
}

auto trc::AssetRegistry::addDefaultValues(const AssetRegistryCreateInfo& info)
    -> AssetRegistryCreateInfo
{
    auto result = info;

    result.materialDescriptorStages |= vk::ShaderStageFlagBits::eFragment;
    result.textureDescriptorStages |= vk::ShaderStageFlagBits::eFragment;

    if (info.enableRayTracing)
    {
        result.geometryBufferUsage |=
            vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress
            | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;

        result.materialDescriptorStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS
                                           | vk::ShaderStageFlagBits::eCompute;
        result.textureDescriptorStages  |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS
                                           | vk::ShaderStageFlagBits::eCompute;
        result.geometryDescriptorStages |= rt::ALL_RAY_PIPELINE_STAGE_FLAGS
                                           | vk::ShaderStageFlagBits::eCompute;
    }

    return result;
}

void trc::AssetRegistry::createDescriptors()
{
    descSet = {};
    descLayout = {};
    descPool = {};

    // Create pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eStorageBuffer, 1 },
        { vk::DescriptorType::eCombinedImageSampler, MAX_TEXTURE_COUNT },
    };
    if (config.enableRayTracing)
    {
        // Index and vertex buffers
        poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, MAX_GEOMETRY_COUNT);
        poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, MAX_GEOMETRY_COUNT);
    }

    descPool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
        | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        1, poolSizes
    });

    // Create descriptor layout
    std::vector<vk::DescriptorBindingFlags> bindingFlags{
        {},                                              // Flags for material buffer
        vk::DescriptorBindingFlagBits::ePartiallyBound,  // Flags for textures
    };
    if (config.enableRayTracing)
    {
        // Flags for vertex- and index buffers
        bindingFlags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
        bindingFlags.emplace_back(vk::DescriptorBindingFlagBits::ePartiallyBound);
    }

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        vk::DescriptorSetLayoutBinding(
            DescBinding::eMaterials,
            vk::DescriptorType::eStorageBuffer,
            1,
            config.materialDescriptorStages
        ),
        vk::DescriptorSetLayoutBinding(
            DescBinding::eTextures,
            vk::DescriptorType::eCombinedImageSampler,
            MAX_TEXTURE_COUNT,
            config.textureDescriptorStages
        ),
    };
    if (config.enableRayTracing)
    {
        layoutBindings.push_back(vk::DescriptorSetLayoutBinding(
            DescBinding::eVertexBuffers,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            config.geometryDescriptorStages
        ));
        layoutBindings.push_back(vk::DescriptorSetLayoutBinding(
            DescBinding::eIndexBuffers,
            vk::DescriptorType::eStorageBuffer,
            MAX_GEOMETRY_COUNT,
            config.geometryDescriptorStages
        ));
    }

    vk::StructureChain layoutChain{
        vk::DescriptorSetLayoutCreateInfo(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
            layoutBindings
        ),
        vk::DescriptorSetLayoutBindingFlagsCreateInfo(bindingFlags)
    };
    descLayout = device->createDescriptorSetLayoutUnique(
        layoutChain.get<vk::DescriptorSetLayoutCreateInfo>()
    );

    // Create descriptor set
    descSet = std::move(device->allocateDescriptorSetsUnique({ *descPool, 1, &*descLayout })[0]);

    // Update provider
    descriptorProvider.setDescriptorSetLayout(*descLayout);
    descriptorProvider.setDescriptorSet(*descSet);
}

void trc::AssetRegistry::writeDescriptors()
{
    const auto& device = instance.getDevice();

    // Material descriptor infos
    vk::DescriptorBufferInfo matBufferWrite(*materialBuffer, 0, VK_WHOLE_SIZE);

    // Collect descriptor writes
    std::vector<vk::WriteDescriptorSet> writes;

    // Bind material buffer
    writes.push_back(vk::WriteDescriptorSet(
        *descSet,
        DescBinding::eMaterials, 0, 1,
        vk::DescriptorType::eStorageBuffer,
        {},
        &matBufferWrite
    ));

    // Texture descriptor infos
    std::vector<vk::DescriptorImageInfo> imageWrites;
    for (ui32 i = 0; i < textures.size(); i++)
    {
        auto& tex = textures[TextureID::ID(i)];
        if (tex == nullptr) break;

        imageWrites.emplace_back(vk::DescriptorImageInfo(
            tex->image.getDefaultSampler(),
            *tex->imageView,
            vk::ImageLayout::eGeneral
        ));
    }

    if (textures.size() > 0)
    {
        writes.push_back(vk::WriteDescriptorSet(
            *descSet,
            DescBinding::eTextures, 0, imageWrites.size(),
            vk::DescriptorType::eCombinedImageSampler,
            imageWrites.data()
        ));
    }

    // Geometry descriptor infos
    std::vector<vk::DescriptorBufferInfo> vertBufs;
    std::vector<vk::DescriptorBufferInfo> indexBufs;
    if (config.enableRayTracing)
    {
        for (auto& geo : geometries)
        {
            if (geo == nullptr)
            {
                vertBufs.emplace_back(vk::DescriptorBufferInfo(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE));
                indexBufs.emplace_back(vk::DescriptorBufferInfo(VK_NULL_HANDLE, 0, VK_WHOLE_SIZE));
            }
            else
            {
                vertBufs.emplace_back(vk::DescriptorBufferInfo(*geo->vertexBuf, 0, VK_WHOLE_SIZE));
                indexBufs.emplace_back(vk::DescriptorBufferInfo(*geo->indexBuf, 0, VK_WHOLE_SIZE));
            }
        }

        if (geometries.size() > 0)
        {
            writes.push_back(vk::WriteDescriptorSet(
                *descSet, DescBinding::eVertexBuffers, 0, vertBufs.size(),
                vk::DescriptorType::eStorageBuffer,
                nullptr,
                vertBufs.data()
            ));
            writes.push_back(vk::WriteDescriptorSet(
                *descSet, DescBinding::eIndexBuffers, 0, indexBufs.size(),
                vk::DescriptorType::eStorageBuffer,
                nullptr,
                indexBufs.data()
            ));
        }
    }

    if (!writes.empty()) {
        device->updateDescriptorSets(writes, {});
    }
}
