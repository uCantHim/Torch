#pragma once

#include "data_utils/SelfManagedObject.h"
#include "RenderPass.h"

namespace trc
{
    class RenderStage : public data::SelfManagedObject<RenderStage>
    {
    public:
        RenderStage(ui32 numSubPasses);
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
