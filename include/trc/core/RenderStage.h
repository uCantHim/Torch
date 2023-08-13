#pragma once

#include <trc_util/data/TypesafeId.h>

namespace trc
{
    /**
     * Create render stages via `RenderStage::make`, which allocates a unique
     * ID to the stage.
     */
    struct RenderStage
    {
        using ID = data::TypesafeID<RenderStage>;

        RenderStage(const RenderStage&) = delete;
        RenderStage(RenderStage&&) noexcept = delete;
        auto operator=(const RenderStage&) -> RenderStage& = delete;
        auto operator=(RenderStage&&) noexcept -> RenderStage& = delete;

        ~RenderStage() = default;

        operator ID() const {
            return id;
        }

        /**
         * @brief Create a render stage with a globally unique ID
         */
        static inline auto make() -> RenderStage
        {
            static ui32 nextId{ 0 };
            return RenderStage{ ID{nextId++} };
        }

    private:
        RenderStage(ID id) : id(id) {}
        ID id;
    };
} // namespace trc
