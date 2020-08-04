#pragma once

#include "base/SceneRegisterable.h"
#include "base/DrawableStatic.h"
#include "Node.h"
#include "PipelineDefinitions.h"

#include "Geometry.h"
#include "AnimationEngine.h"
#include "PickableRegistry.h"

namespace trc
{
    using namespace internal;

    class Drawable : public Node
    {
    public:
        Drawable(Geometry& geo, ui32 material, SceneBase& scene);
        ~Drawable();

        Drawable(const Drawable&) = delete;
        Drawable(Drawable&&) noexcept = default;
        auto operator=(const Drawable&) -> Drawable& = delete;
        auto operator=(Drawable&&) noexcept -> Drawable& = default;

        void setGeometry(Geometry& geo);
        void setMaterial(ui32 matIndex);

        template<typename PickableType, typename ...Args>
        auto enablePicking(Args&&... args) -> PickableType&
        {
            PickableType& newPickable = PickableRegistry::makePickable<PickableType, Args...>(
                std::forward<Args>(std::move(args))...
            );

            pickableId = newPickable.getPickableId();
            updateDrawFunction();

            return newPickable;
        }
        auto getAnimationEngine() noexcept -> AnimationEngine&;

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

        Geometry* geo{ nullptr };
        ui32 matIndex{ 0 };
        Pickable::ID pickableId{ NO_PICKABLE };

        AnimationEngine animEngine;
    };
}
