#pragma once

#include "Types.h"
#include "util/data/ObjectId.h"

namespace trc
{
    struct RenderStage
    {
        using ID = TypesafeID<RenderStage>;

        RenderStage() = default;
        ~RenderStage() = default;

        RenderStage(const RenderStage&) = delete;
        RenderStage(RenderStage&&) noexcept = delete;

        auto operator=(const RenderStage&) -> RenderStage& = delete;
        auto operator=(RenderStage&&) noexcept -> RenderStage& = delete;

        operator ID() const {
            return id;
        }

    private:
        static inline data::IdPool idPool;

        TypesafeID<RenderStage> id{ idPool.generate() };
    };
} // namespace trc
