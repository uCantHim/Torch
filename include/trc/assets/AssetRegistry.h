#pragma once

#include <string>
#include <unordered_map>
#include <atomic>

#include <trc_util/Exception.h>
#include <trc_util/data/IndexMap.h>
#include <trc_util/functional/Maybe.h>
#include "trc/base/Image.h"
#include "trc/base/MemoryPool.h"

#include "trc/Types.h"
#include "trc/UpdatePass.h"
#include "trc/assets/AssetRegistryModuleStorage.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/SharedDescriptorSet.h"
#include "trc/core/DescriptorProvider.h"

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

        /**
         * @brief Add an asset to the registry
         */
        template<AssetBaseType T>
        auto add(u_ptr<AssetSource<T>> source) -> LocalID<T>;

        /**
         * @brief Retrive a handle to an asset's device data
         *
         * @throw std::out_of_range if no module for `T` has been registered
         *        previously.
         */
        template<AssetBaseType T>
        auto get(LocalID<T> key) -> AssetHandle<T>
        {
            return getModule<T>().getHandle(key);
        }

        /**
         * @brief Remove an asset from the registry
         *
         * @throw std::out_of_range if no module for `T` has been registered
         *                          previously.
         */
        template<AssetBaseType T>
        void remove(LocalID<T> id);

        /**
         * @brief Register a module at the asset registry
         *
         * A module is an object that manages device representation for a
         * single type of asset. An example is the GeometryRegistry for
         * geometries.
         *
         * Asset management is delegated to single modules, while the asset
         * registry manages the modules and provides a single unified
         * interface to them.
         *
         * @throw std::out_of_range if a module for `T` has already been
         *                          registered.
         */
        template<AssetBaseType T, typename... Args>
            requires requires { typename AssetRegistryModule<T>; }
            && std::derived_from<AssetRegistryModule<T>, AssetRegistryModuleInterface<T>>
        void addModule(Args&&... args)
        {
            modules.addModule<T>(std::forward<Args>(args)...);
        }

        /**
         * @throw std::out_of_range
         */
        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModuleInterface<T>&;

        /**
         * @throw std::out_of_range
         */
        template<AssetBaseType T>
        auto getModule() const -> const AssetRegistryModuleInterface<T>&;

        auto getUpdatePass() -> UpdatePass&;
        auto getDescriptorSetProvider() const noexcept -> const DescriptorProviderInterface&;

    private:
        static auto addDefaultValues(AssetRegistryCreateInfo info)
            -> AssetRegistryCreateInfo;

        const Device& device;
        const AssetRegistryCreateInfo config;

        AssetRegistryModuleStorage modules;


        //////////////
        // Descriptors

        struct AssetModuleUpdatePass : UpdatePass
        {
            AssetModuleUpdatePass(AssetRegistry* reg) : registry(reg) {}
            void update(vk::CommandBuffer cmdBuf, FrameRenderState& frameState) override;

            AssetRegistry* registry;
        };

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& frameState);

        u_ptr<SharedDescriptorSet> descSet;
        u_ptr<AssetModuleUpdatePass> updateRenderPass{ new AssetModuleUpdatePass{ this } };
    };



    template<AssetBaseType T>
    auto AssetRegistry::add(u_ptr<AssetSource<T>> source) -> LocalID<T>
    {
        return getModule<T>().add(std::move(source));
    }

    template<AssetBaseType T>
    inline void AssetRegistry::remove(LocalID<T> id)
    {
        getModule<T>().remove(id);
    }

    template<AssetBaseType T>
    inline auto AssetRegistry::getModule() -> AssetRegistryModuleInterface<T>&
    {
        return modules.get<T>();
    }

    template<AssetBaseType T>
    inline auto AssetRegistry::getModule() const -> const AssetRegistryModuleInterface<T>&
    {
        return modules.get<T>();
    }
} // namespace trc
