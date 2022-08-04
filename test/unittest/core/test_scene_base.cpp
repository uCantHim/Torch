#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <trc/core/SceneBase.h>
using namespace trc;

TEST(SceneBaseTest, ConstructDestruct)
{
    SceneBase scene;
}

TEST(SceneBaseTest, DrawFunctionExecution)
{
    SceneBase scene;

    std::vector<RenderStage::ID> s{ RenderStage::ID(0), RenderStage::ID(1), RenderStage::ID(2) };
    std::vector<SubPass::ID>     u{ SubPass::ID(0), SubPass::ID(1), SubPass::ID(2) };
    std::vector<Pipeline::ID>    p{ Pipeline::ID(0), Pipeline::ID(5), Pipeline::ID(42),
                                    Pipeline::ID(7777), Pipeline::ID(108) };

    std::vector<SceneBase::RegistrationID> regs;
    regs.reserve(1000);

    size_t numExecuted{ 0 };
    auto func = [&](auto&&, auto&&) {
        ++numExecuted;
    };

    for (int i = 0; i < 9; ++i)
    {
        for (int j = 0; j < 9; ++j)
        {
            for (int k = 0; k < 1000; ++k)
            {
                auto id = scene.registerDrawFunction(s[i % 3], u[j % 3], p[k % 5], func);
                regs.emplace_back(std::move(id));
            }
        }
    }

    ASSERT_EQ(scene.iterPipelines(s[0], u[0]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[0], u[1]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[0], u[2]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[1], u[0]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[1], u[1]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[1], u[2]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[2], u[0]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[2], u[1]).size(), 5);
    ASSERT_EQ(scene.iterPipelines(s[2], u[2]).size(), 5);

    scene.invokeDrawFunctions(s[1], *(RenderPass*)nullptr, u[0], p[3], *(Pipeline*)nullptr, {});
    ASSERT_EQ(numExecuted, (3 * 3 * 1000) / 5);

    numExecuted = 0;
    scene.invokeDrawFunctions(s[0], *(RenderPass*)nullptr, u[2], p[1], *(Pipeline*)nullptr, {});
    ASSERT_EQ(numExecuted, (3 * 3 * 1000) / 5);

    ASSERT_THROW(
        scene.invokeDrawFunctions(
            RenderStage::ID(200), *(RenderPass*)nullptr, u[2], p[0], *(Pipeline*)nullptr, {}
        ),
        std::out_of_range
    );

    for (auto id : regs) {
        scene.unregisterDrawFunction(id);
    }

    ASSERT_TRUE(scene.iterPipelines(s[0], u[0]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[0], u[1]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[0], u[2]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[1], u[0]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[1], u[1]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[1], u[2]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[2], u[0]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[2], u[1]).empty());
    ASSERT_TRUE(scene.iterPipelines(s[2], u[2]).empty());

    numExecuted = 0;
    scene.invokeDrawFunctions(s[0], *(RenderPass*)nullptr, u[2], p[1], *(Pipeline*)nullptr, {});
    ASSERT_EQ(numExecuted, 0);
}

TEST(SceneBaseTest, ThreadSafeRegistration)
{
    SceneBase scene;

    std::vector<RenderStage::ID> s{ RenderStage::ID(0), RenderStage::ID(2), RenderStage::ID(4) };
    std::vector<SubPass::ID>     u{ SubPass::ID(0), SubPass::ID(1), SubPass::ID(2) };
    std::vector<Pipeline::ID>    p{ Pipeline::ID(0), Pipeline::ID(1), Pipeline::ID(2),
                                    Pipeline::ID(3), Pipeline::ID(4) };

    size_t numInvocations{ 0 };
    auto func = [&](auto&&, auto&&){ ++numInvocations; };

    // Register draw functions from multiple threads
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 100; ++i)
    {
        threads.emplace_back([&]{
            for (auto s_ : s) {
                for (auto u_ : u) {
                    for (auto p_ : p) {
                        scene.registerDrawFunction(s_, u_, p_, func);
                    }
                }
            }
        });
    }
    for (auto& t : threads) t.join();

    ASSERT_EQ(scene.iterPipelines(s[0], u[0]).size(), 5);

    scene.invokeDrawFunctions(s[0], *(RenderPass*)nullptr, u[0], p[0], *(Pipeline*)nullptr, {});
    ASSERT_EQ(numInvocations, 100);
}
