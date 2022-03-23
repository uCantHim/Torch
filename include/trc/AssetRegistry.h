#pragma once

#include <string>
#include <unordered_map>
#include <atomic>

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <trc_util/Exception.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/functional/Maybe.h>

#include "Types.h"
#include "core/DescriptorProvider.h"
#include "AssetRegistryModuleStorage.h"
#include "Assets.h"
#include "AssetSource.h"
#include "assets/RawData.h"
#include "Geometry.h"
#include "Material.h"
#include "Rig.h"
#include "AnimationDataStorage.h"
#include "text/FontDataStorage.h"

namespace trc
{
    class Instance;

    class DuplicateKeyError : public Exception {};
    class KeyNotFoundError : public Exception {};

    struct AssetRegistryCreateInfo
    {
        vk::BufferUsageFlags geometryBufferUsage{};

        vk::ShaderStageFlags materialDescriptorStages{};
        vk::ShaderStageFlags textureDescriptorStages{};
        vk::ShaderStageFlags geometryDescriptorStages{};

        bool enableRayTracing{ true };
    };

    class AssetRegistry
    {
    public:
        template<AssetBaseType T>
        using LocalID = typename TypedAssetID<T>::LocalID;

        explicit AssetRegistry(const Instance& instance,
                               const AssetRegistryCreateInfo& info = {});

        auto add(u_ptr<AssetSource<Geometry>> geo) -> LocalID<Geometry>;
        auto add(u_ptr<AssetSource<Material>> mat) -> LocalID<Material>;
        auto add(u_ptr<AssetSource<Texture>>tex) -> LocalID<Texture>;

        template<AssetBaseType T>
        auto get(LocalID<T> key) -> AssetHandle<T>
        {
            return getModule<T>().getHandle(key);
        }

        template<AssetBaseType T>
        void remove(LocalID<T> id);

        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModule<T>&;
        template<AssetBaseType T>
        auto getModule() const -> const AssetRegistryModule<T>&;

        auto getFonts() -> FontDataStorage&;
        auto getFonts() const -> const FontDataStorage&;
        auto getAnimations() -> AnimationDataStorage&;
        auto getAnimations() const -> const AnimationDataStorage&;

        auto getDescriptorSetProvider() const noexcept -> const DescriptorProviderInterface&;

        // TODO: Don't re-upload ALL materials every time one is added
        void updateMaterials();

    private:
        static auto addDefaultValues(const AssetRegistryCreateInfo& info) -> AssetRegistryCreateInfo;

        const vkb::Device& device;
        const AssetRegistryCreateInfo config;

        AssetRegistryModuleStorage modules;

        //////////////
        // Descriptors
        enum DescBinding
        {
            eMaterials = 0,
            eTextures = 1,
            eVertexBuffers = 2,
            eIndexBuffers = 3,
        };

        void createDescriptors();
        void writeDescriptors();

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;
        DescriptorProvider descriptorProvider{ {}, {} };

        ////////////////////////////
        // Additional asset storages
        FontDataStorage fontData;
        AnimationDataStorage animationStorage;
    };



    template<AssetBaseType T>
    inline void AssetRegistry::remove(LocalID<T> id)
    {
        modules.get<AssetRegistryModule<T>>().remove(id);
    }

    template<AssetBaseType T>
    inline auto AssetRegistry::getModule() -> AssetRegistryModule<T>&
    {
        return modules.get<AssetRegistryModule<T>>();
    }

    template<AssetBaseType T>
    inline auto AssetRegistry::getModule() const -> const AssetRegistryModule<T>&
    {
        return modules.get<AssetRegistryModule<T>>();
    }
} // namespace trc
