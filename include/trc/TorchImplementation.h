#pragma once

#include "trc/AssetDescriptor.h"

namespace trc
{
    class Instance;
    class AssetRegistry;
} // namespace trc

namespace trc::impl
{
    /**
     * Register all of Torch's assets at an AssetRegistry and create the
     * corresponding asset descriptor set. This may only be done once for an
     * `AssetRegistry` object.
     *
     * @throw std::invalid_argument if `makeDefaultAssetModules` has already
     *                              been called on `registry`.
     */
    auto makeDefaultAssetModules(const Instance& instance,
                                 AssetRegistry& registry,
                                 const AssetDescriptorCreateInfo& descriptorCreateInfo)
        -> s_ptr<AssetDescriptor>;
} // namespace trc::impl
