#pragma once

#include "base/SceneRegisterable.h"
#include "base/DrawableStatic.h"
#include "Node.h"
#include "PipelineDefinitions.h"

#include "Geometry.h"
#include "AnimationEngine.h"

namespace trc
{
    using namespace internal;

    class Drawable : public Node
    {
    public:
        Drawable(Geometry& geo, ui32 material, SceneBase& scene);

        void setGeometry(Geometry& geo);
        void setMaterial(ui32 matIndex);

        void makePickable();
        auto getAnimationEngine() noexcept -> AnimationEngine& {
            return animEngine;
        }

        void attachToScene(SceneBase& scene);

    private:
        void updateDrawFunction();

        void prepareDraw(vk::CommandBuffer cmdBuf, vk::PipelineLayout layout);

        void draw(vk::CommandBuffer cmdBuf);
        void drawAnimated(vk::CommandBuffer cmdBuf);
        void drawPickable(vk::CommandBuffer cmdBuf);
        void drawAnimatedAndPickable(vk::CommandBuffer cmdBuf);

        SceneBase* currentScene{ nullptr };
        SceneBase::RegistrationID registration;
        bool isPickable{ false };

        Geometry* geo{ nullptr };
        ui32 matIndex{ 0 };

        AnimationEngine animEngine;
    };
}
