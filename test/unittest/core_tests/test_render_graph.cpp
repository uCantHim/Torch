#include <deque>

#include <gtest/gtest.h>

#include <trc/core/RenderGraph.h>

using trc::RenderGraph;
using trc::RenderStage;
using trc::makeRenderStage;

struct RenderGraphTest : public testing::Test
{
    RenderStage::ID a = makeRenderStage();
    RenderStage::ID b = makeRenderStage();
    RenderStage::ID c = makeRenderStage();
    RenderStage::ID d = makeRenderStage();
    RenderStage::ID e = makeRenderStage();
    RenderStage::ID f = makeRenderStage();
    RenderStage::ID g = makeRenderStage();
    RenderStage::ID h = makeRenderStage();
    RenderStage::ID i = makeRenderStage();
};

/**
 * Utility that tests stage ordering while iterating over a render graph
 */
void testDeps(const RenderGraph& graph,
              const std::unordered_map<RenderStage::ID, std::vector<RenderStage::ID>>& deps)
{
    const auto ordering = graph.compile();

    std::unordered_set<RenderStage::ID> visited;
    for (auto stage : ordering)
    {
        for (const auto& depStage : deps.at(stage)) {
            ASSERT_TRUE(visited.contains(depStage));
        }
        visited.emplace(stage);
    }
};

TEST_F(RenderGraphTest, DoubleInsertDoesNotThrow)
{
    RenderGraph graph;
    graph.insert(a);
    graph.insert(b);
    graph.insert(c);
    ASSERT_NO_THROW(graph.insert(a));
    ASSERT_NO_THROW(graph.insert(b));
    ASSERT_NO_THROW(graph.insert(c));
}

TEST_F(RenderGraphTest, IterateOverGraph)
{
    // Iterate over empty graph
    RenderGraph graph;
    auto iter = graph.compile();
    ASSERT_EQ(iter.begin(), iter.end());

    int i{ 0 };
    for (const auto& stage : iter) {
        ++i;
    }
    ASSERT_EQ(i, 0);

    // Add some elements
    graph.insert(a);
    iter = graph.compile();
    ASSERT_EQ(++iter.begin(), iter.end());

    graph.insert(b);
    graph.insert(c);
    iter = graph.compile();
    ASSERT_EQ(++++++iter.begin(), iter.end());
    for (const auto& stage : iter) {
        ++i;
    }
    ASSERT_EQ(i, 3);

    graph.insert(f);
    iter = graph.compile();
    for (const auto& stage : iter) {
        ++i;
    }
    ASSERT_EQ(i, 7);
}

TEST_F(RenderGraphTest, StageOrder)
{
    // Trivial linear
    {
        RenderGraph graph;
        graph.insert(a);
        graph.insert(b);
        graph.insert(c);
        graph.insert(d);
        graph.insert(e);
        graph.insert(f);
        graph.insert(g);
        graph.createOrdering(a, b);
        graph.createOrdering(b, c);
        graph.createOrdering(c, d);
        graph.createOrdering(d, e);
        graph.createOrdering(e, f);
        graph.createOrdering(f, g);

        testDeps(graph, {
            { a, {} },
            { b, { a } },
            { c, { a, b } },
            { d, { a, b, c } },
            { e, { a, b, c, d } },
            { f, { a, b, c, d, e } },
            { g, { a, b, c, d, e, f } },
        });
    }

    // Simple branch with merge
    {
        RenderGraph graph;
        graph.insert(a);
        graph.insert(b);
        graph.insert(c);
        graph.createOrdering(a, c);
        graph.createOrdering(a, b);
        graph.createOrdering(b, c);

        testDeps(graph, {
            { a, {} },
            { b, { a } },
            { c, { a, b } },
        });
    }

    // More complex branch with merge
    {
        RenderGraph graph;
        graph.insert(a);
        graph.insert(b);
        graph.insert(c);
        graph.insert(d);
        graph.createOrdering(a, b);
        graph.createOrdering(c, b);
        graph.createOrdering(b, d);
        graph.createOrdering(a, c);

        testDeps(graph, {
            { a, {} },
            { b, { a, c } },
            { c, { a } },
            { d, { a, b, c } },
        });
    }

    // Complex graph layout and shuffled edge insertion
    {
        RenderGraph graph;
        graph.insert(a);
        graph.insert(b);
        graph.insert(c);
        graph.insert(d);
        graph.insert(e);
        graph.insert(f);
        graph.insert(g);
        graph.insert(h);

        // A shuffled list of edges
        graph.createOrdering(d, g);
        graph.createOrdering(a, b);
        graph.createOrdering(c, e);
        graph.createOrdering(h, g);
        graph.createOrdering(c, d);
        graph.createOrdering(e, h);
        graph.createOrdering(a, c);
        graph.createOrdering(c, f);
        graph.createOrdering(b, e);

        testDeps(graph, {
            { a, {} },
            { b, { a } },
            { c, { a } },
            { d, { a, c } },
            { e, { a, b, c } },
            { f, { a, c } },
            { g, { a, c, d, h } },
            { h, { a, c, e } },
        });
    }
}

TEST_F(RenderGraphTest, CycleDetection)
{
    RenderGraph graph;
    graph.insert(a);
    graph.insert(b);
    graph.insert(c);
    graph.createOrdering(a, b);
    graph.createOrdering(c, a);
    ASSERT_THROW(graph.createOrdering(b, c), std::invalid_argument);

    // Check if graph is still functional
    std::deque<RenderStage::ID> stages{ c, a, b };
    for (auto stage : graph.compile()) {
        ASSERT_EQ(stage, stages.front());
        stages.pop_front();
    }
    ASSERT_TRUE(stages.empty());

    // Create loop that doesn't include the head stage
    graph.insert(d);
    graph.createOrdering(b, d);
    ASSERT_THROW(graph.createOrdering(d, a), std::invalid_argument);
}

TEST_F(RenderGraphTest, CreateOrderingInsertsNewStages)
{
    RenderGraph graph;
    graph.insert(a);
    graph.createOrdering(c, b);

    std::unordered_set<RenderStage::ID> stages{ a, b, c };
    for (auto stage : graph.compile()) {
        ASSERT_TRUE(stages.erase(stage));
    }
    ASSERT_TRUE(stages.empty());
}
