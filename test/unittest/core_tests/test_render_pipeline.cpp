#include <array>

#include <gtest/gtest.h>

#include <trc/Torch.h>
#include <trc/core/Instance.h>
#include <trc/core/RenderPipeline.h>

using namespace trc;

class RenderPipelineTest : public testing::Test
{
protected:
    RenderPipelineTest()
        :
        instance([]{
            trc::init();
            return InstanceCreateInfo{ .enableRayTracing=false };
        }()),
        frameClock(1),
        renderTarget{
            frameClock,
            { VK_NULL_HANDLE }, { VK_NULL_HANDLE },
            RenderTargetCreateInfo{ {1, 1}, vk::Format::eR8G8B8A8Unorm, {} },
        }
    {}

    ~RenderPipelineTest() {
        trc::terminate();
    }

    auto makePipeline(const ui32 maxViewports = 1)
        -> u_ptr<RenderPipeline>
    {
        return trc::buildRenderPipeline().build(
            trc::RenderPipelineCreateInfo{
                .instance=instance,
                .renderTarget=renderTarget,
                .maxViewports=maxViewports
            }
        );
    }

    Instance instance;
    FrameClock frameClock;
    RenderTarget renderTarget;

    Camera camera;
    SceneBase scene;
};

TEST_F(RenderPipelineTest, ViewportCountLimitations)
{
    using TooManyViewports = std::out_of_range;

    // Pipeline with a single viewport
    {
        auto pipeline = makePipeline();
        auto vp = pipeline->makeViewport({}, camera, scene);
        ASSERT_NE(vp, nullptr);
        ASSERT_THROW(pipeline->makeViewport({}, camera, scene), TooManyViewports);

        vp.reset();
        ASSERT_NO_THROW(pipeline->makeViewport({}, camera, scene));
        ASSERT_NO_THROW(vp = pipeline->makeViewport({}, camera, scene));
        ASSERT_THROW(vp = pipeline->makeViewport({}, camera, scene), TooManyViewports);
        ASSERT_THROW(pipeline->makeViewport({}, camera, scene), TooManyViewports);
    }

    // Pipeline with multiple viewports
    {
        constexpr ui32 maxViewports = 12;
        std::array<ViewportHandle, maxViewports> vps;

        auto pipeline = makePipeline(maxViewports);
        for (auto& vp : vps)
        {
            vp = pipeline->makeViewport({}, camera, scene);
            ASSERT_NE(vp, nullptr);
        }
        ASSERT_THROW(pipeline->makeViewport({}, camera, scene), TooManyViewports);
        vps.at(0).reset();
        vps.at(3).reset();
        ASSERT_NO_THROW(vps[0] = pipeline->makeViewport({}, camera, scene));
        ASSERT_NO_THROW(vps[3] = pipeline->makeViewport({}, camera, scene));
        ASSERT_THROW(pipeline->makeViewport({}, camera, scene), TooManyViewports);

        vps = {};
        for (ui32 i = 0; i < maxViewports * 2; ++i) {
            ASSERT_NO_THROW(pipeline->makeViewport({}, camera, scene));
        }
    }
}
