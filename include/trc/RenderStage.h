#pragma once

#include <atomic>

#include "data_utils/SelfManagedObject.h"
#include "RenderPass.h"

namespace trc
{
    struct RenderStageType : public data::SelfManagedObject<RenderStageType>
    {
    public:
        RenderStageType(ui32 numSubPasses) : numSubPasses(numSubPasses) {}

        const ui32 numSubPasses;
    };

    /**
     * @brief A collection of render passes
     */
    class RenderStage : public data::SelfManagedObject<RenderStage>
    {
    public:
        explicit RenderStage(RenderStageType& type);
        virtual ~RenderStage() = default;

        virtual auto getRenderPasses() const noexcept -> const std::vector<RenderPass::ID>&;
        virtual void addRenderPass(RenderPass::ID newPass);
        virtual void removeRenderPass(RenderPass::ID pass);

        auto getNumSubPasses() const noexcept -> ui32;

    protected:
        const ui32 numSubPasses;
        std::vector<RenderPass::ID> renderPasses;
    };
} // namespace trc
