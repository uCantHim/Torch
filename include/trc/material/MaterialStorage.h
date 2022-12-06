#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "MaterialRuntime.h"
#include "trc/Types.h"

namespace trc
{
    struct MaterialInfo
    {
        ShaderModule fragmentModule;
        ShaderDescriptorConfig descriptorConfig;
        PipelineFragmentParams fragmentInfo;
    };

    using MatID = ui32;

    struct MaterialKey
    {
        bool operator==(const MaterialKey& rhs) const {
            return vertexParams.animated == rhs.vertexParams.animated;
        }

        PipelineVertexParams vertexParams;
    };
}

template<>
struct std::hash<trc::MaterialKey>
{
    auto operator()(const trc::MaterialKey& key) const -> size_t
    {
        return std::hash<bool>{}(key.vertexParams.animated);
    }
};

namespace trc
{
    class MaterialStorage
    {
    public:
        auto registerMaterial(MaterialInfo info) -> MatID;
        void removeMaterial(MatID id);

        auto getFragmentParams(MatID id) const -> const PipelineFragmentParams&;
        auto getRuntime(MatID id, PipelineVertexParams params) -> MaterialRuntimeInfo&;

    private:
        class MaterialFactory
        {
        public:
            MaterialFactory(MaterialFactory&&) = default;

            explicit MaterialFactory(MaterialInfo info);

            auto getInfo() const -> const MaterialInfo&;
            auto getOrMake(MaterialKey specialization) -> MaterialRuntimeInfo&;

            /**
             * Free all material runtimes. Keep the create info in storage.
             */
            void clear();

        private:
            MaterialInfo materialCreateInfo;
            std::unordered_map<MaterialKey, u_ptr<MaterialRuntimeInfo>> runtimes;
        };

        std::vector<MaterialFactory> materialFactories;
    };
} // namespace trc
