#pragma once

#include <trc_util/data/TypesafeId.h>

#include "trc/Types.h"

namespace trc
{
    struct RenderStage;

    /** @brief Create a unique render stage */
    auto makeRenderStage(const char* name =  "<unnamed_render_stage>") -> RenderStage;

    /**
     * Create render stages via `RenderStage::make`, which allocates a unique
     * ID to the stage.
     */
    struct RenderStage
    {
        using ID = data::TypesafeID<RenderStage>;

        operator ID() const {
            return id;
        }

        bool operator==(const RenderStage& other) const {
            return id == other.id;
        }

        auto getID() const -> ID {
            return id;
        }

        auto getDescription() const {
            return description;
        }

        /**
         * @brief Create a render stage with a globally unique ID
         */
        static auto make(const char* name = "<unnamed_render_stage>")
            -> RenderStage
        {
            static ui32 nextId{ 0 };
            return RenderStage{ ID{nextId++}, name };
        }

    private:
        constexpr RenderStage(ID id, const char* description)
            : id(id), description(description) {}

        ID id;
        const char* description;
    };

    inline auto makeRenderStage(const char* name) -> RenderStage {
        return RenderStage::make(name);
    }
} // namespace trc

template<>
struct std::hash<trc::RenderStage>
{
    auto operator()(const trc::RenderStage& stage) const -> size_t {
        return std::hash<trc::RenderStage::ID>{}(stage);
    }
};
