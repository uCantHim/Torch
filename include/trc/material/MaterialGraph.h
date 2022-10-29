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
     */
    class MaterialGraph
    {
    public:
        MaterialGraph() = default;

        auto makeConstant(Constant c) -> MaterialNode*;
        auto makeBuiltinConstant(Builtin type) -> MaterialNode*;

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
    };
} // namespace trc
