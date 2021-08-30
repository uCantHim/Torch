#include "AssetRegistry.h"

#include <vkb/ImageUtils.h>

#include "core/Instance.h"



trc::AssetRegistry::AssetRegistry(const Instance& instance)
    :
    instance(instance),
    device(instance.getDevice()),
    memoryPool(instance.getDevice(), MEMORY_POOL_CHUNK_SIZE),
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
    add(Material{});
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
                data.indices,
                // Just always specify the shader device address flag in case I
                // want to use the geometry for ray tracing. Doesn't hurt even if
                // the feature is not enabled.
                vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eShaderDeviceAddress
                | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                memoryPool.makeAllocator()
            },
            .vertexBuf = {
                device,
                data.vertices,
                vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eShaderDeviceAddress
                | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                memoryPool.makeAllocator()
            },
            .numIndices = static_cast<ui32>(data.indices.size()),
            .numVertices = static_cast<ui32>(data.vertices.size()),

            .rig = rigData.has_value()
                ? std::optional<Rig>(Rig(rigData.value(), animationStorage))
                : std::nullopt,
        }
    );

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

    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        vk::DescriptorSetLayoutBinding(
            MAT_BUFFER_BINDING,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment
        ),
        vk::DescriptorSetLayoutBinding(
            IMG_DESCRIPTOR_BINDING,
            vk::DescriptorType::eCombinedImageSampler,
            MAX_TEXTURE_COUNT,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        ),
    };

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

    // Collect descriptor writes
    std::vector<vk::WriteDescriptorSet> writes;
    writes.push_back(vk::WriteDescriptorSet(
        *descSet,
        MAT_BUFFER_BINDING, 0, 1,
        vk::DescriptorType::eStorageBuffer,
        {},
        &matBufferWrite
    ));

    // Only write texture information if at least one texture would be
    // written (required by Vulkan)
    if (textures.size() > 0)
    {
        device->updateDescriptorSets(vk::WriteDescriptorSet(
            *descSet,
            IMG_DESCRIPTOR_BINDING, 0, imageWrites.size(),
            vk::DescriptorType::eCombinedImageSampler,
            imageWrites.data()
        ), {});
    }

    if (!writes.empty()) {
        device->updateDescriptorSets(writes, {});
    }
}
