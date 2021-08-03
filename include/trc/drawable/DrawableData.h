#pragma once

#include <array>
#include <vector>

#include "Node.h"
#include "Geometry.h"
#include "AssetIds.h"
#include "AnimationEngine.h"
#include "PickableRegistry.h"

namespace trc
{
    struct DrawableData
    {
        Node* node;

        Geometry geo{};
        MaterialID mat{};

        Pickable::ID pickableId{ NO_PICKABLE };
        bool isTransparent{ false };

        AnimationEngine animEngine{};
    };

    class DrawableDataStore
    {
    public:
        static auto create(Node& node) -> ui32;
        static void free(ui32 id);

        static auto get(ui32 id) -> DrawableData&;

    private:
        static inline data::IdPool idPool;

        static constexpr size_t ARRAY_SIZE{ 500 };
        static inline std::vector<u_ptr<std::array<DrawableData, ARRAY_SIZE>>> data;
    };
} // namespace trc
