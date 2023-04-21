#pragma once

#include "trc/VulkanInclude.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetSource.h"

namespace trc
{
    class FrameRenderState;

    /**
     * @brief Type-agnostic part of an asset module's interface
     */
    class AssetRegistryModuleInterfaceCommon
    {
    public:
        AssetRegistryModuleInterfaceCommon() = default;
        virtual ~AssetRegistryModuleInterfaceCommon() = default;

        virtual void update(vk::CommandBuffer cmdBuf, FrameRenderState& state) = 0;
    };

    /**
     * @brief Type-specific part of an asset module's interface
     */
    template<AssetBaseType T>
    class AssetRegistryModuleInterface : public AssetRegistryModuleInterfaceCommon
    {
    public:
        using LocalID = typename AssetBaseTypeTraits<T>::LocalID;
        using Handle = typename AssetBaseTypeTraits<T>::Handle;

        virtual auto add(u_ptr<AssetSource<T>> source) -> LocalID = 0;
        virtual void remove(LocalID id) = 0;

        virtual auto getHandle(LocalID id) -> Handle = 0;
    };
} // namespace trc
