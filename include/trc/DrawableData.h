#pragma once

#include <array>
#include <vector>

#include "utils/Transformation.h"
#include "Geometry.h"
#include "AssetIds.h"
#include "AnimationEngine.h"
#include "PickableRegistry.h"

namespace trc
{
    struct DrawableData
    {
        ui32 matrixId;

        Geometry* geo{ 0 };
        MaterialID mat{ 0 };

        Pickable::ID pickableId{ NO_PICKABLE };
        bool isTransparent{ false };

        AnimationEngine animEngine{};
    };

    class DrawableDataStore
    {
    public:
        static auto create(const Transformation& t) -> ui32;
        static void free(ui32 id);

        static auto get(ui32 id) -> DrawableData&;

    private:
        static inline data::IdPool idPool;

        static constexpr size_t ARRAY_SIZE{ 500 };
        static inline std::vector<u_ptr<std::array<DrawableData, ARRAY_SIZE>>> data;
    };
} // namespace trc
