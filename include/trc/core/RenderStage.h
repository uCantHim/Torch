#pragma once

#include <trc_util/data/TypesafeId.h>
#include <trc_util/data/ObjectId.h>

namespace trc
{
    /**
     * Allocates a unique ID automatically on creation. Can be created as
     * `static` to create global IDs.
     */
    struct RenderStage
    {
        using ID = data::TypesafeID<RenderStage>;

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

        ID id{ idPool.generate() };
    };
} // namespace trc
