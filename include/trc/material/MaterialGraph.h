#pragma once

#include <concepts>
#include <initializer_list>
#include <memory>
#include <vector>

#include "Constant.h"
#include "MaterialFunction.h"
#include "MaterialNode.h"
#include "ShaderResourceInterface.h"
#include "trc/Types.h"

namespace trc
{
    /**
     * @brief A builder interface that creates MaterialNodes
     *
     * This is mostly a pool that holds MaterialNodes. Provides convenience
     * functions that create commonly used types of nodes.
     */
    class MaterialGraph
    {
    public:
        MaterialGraph() = default;

        auto makeConstant(Constant c) -> MaterialNode*;
        auto makeCapabilityAccess(Capability capability, BasicType type) -> MaterialNode*;

        auto makeTextureSample(TextureReference tex, MaterialNode* uvs) -> MaterialNode*;

        template<std::derived_from<MaterialFunction> T>
        auto makeFunction(T&& func, std::initializer_list<MaterialNode*> args) -> MaterialNode*
        {
            return makeNode(std::make_unique<T>(std::forward<T>(func)), args);
        }

    private:
        auto makeNode(u_ptr<MaterialFunction> func, std::initializer_list<MaterialNode*> args)
            -> MaterialNode*;

        std::vector<u_ptr<MaterialNode>> nodes;
        std::unordered_map<Capability, MaterialNode*> cachedCapabilityNodes;
    };
} // namespace trc
