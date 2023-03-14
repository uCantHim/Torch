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
     * corresponding asset descriptor set.
     */
    auto makeDefaultAssetModules(const Instance& instance,
                                 AssetRegistry& registry,
                                 const AssetDescriptorCreateInfo& descriptorCreateInfo)
        -> AssetDescriptor;
} // namespace trc::impl
