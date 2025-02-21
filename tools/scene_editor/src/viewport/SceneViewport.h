#pragma once

#include <trc/Torch.h>
using namespace trc::basic_types;

#include "viewport/InputViewport.h"

class SceneViewport : public InputViewport
{
public:
    SceneViewport(trc::RenderPipeline& pipeline,
                  const s_ptr<trc::Camera>& camera,
                  const s_ptr<trc::Scene>& scene,
                  const ViewportArea& initialArea)
        :
        pipeline(pipeline),
        vp(pipeline.makeViewport({ initialArea.pos, initialArea.size }, camera, scene))
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
        pipeline.draw(vp, frame);
    }

private:
    trc::RenderPipeline& pipeline;
    trc::ViewportHandle vp;
};
