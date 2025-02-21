#pragma once

#include <trc/Torch.h>
using namespace trc::basic_types;

#include "asset/AssetInventory.h"
#include "viewport/InputViewport.h"

class SceneViewport : public InputViewport
{
public:
    SceneViewport(trc::ViewportHandle vp)
        : vp(vp)
    {}

    SceneViewport(trc::Window& window,
                  AssetInventory& assets,
                  const ViewportArea& vpArea,
                  s_ptr<trc::Scene> scene,
                  s_ptr<trc::Camera> camera)
        :
        pipeline(trc::makeTorchRenderPipeline(
            window.getInstance(),
            window,
            trc::TorchPipelineCreateInfo{
                .assetRegistry=assets.manager().getDeviceRegistry(),
                .assetDescriptorCreateInfo={},
            }
        )),
        vp(pipeline->makeViewport(
            trc::RenderArea{ vpArea.pos, vpArea.size },
            camera,
            scene,
            vec4{ 0.3f, 0.3f, 1.0f, 1.0f }
        ))
    {}

    void resize(const ViewportArea& newArea) override
    {
        vp->resize({ .offset=newArea.pos, .size=newArea.size });
        vp->getCamera().setAspect(float(newArea.size.x) / float(newArea.size.y));
    }

    auto getSize() -> ViewportArea override
    {
        return { .pos=vp->getRenderArea().offset, .size=vp->getRenderArea().size };
    }

    void draw(trc::Frame& frame) override
    {
        pipeline->draw(vp, frame);
    }

private:
    u_ptr<trc::RenderPipeline> pipeline;
    trc::ViewportHandle vp;
};
