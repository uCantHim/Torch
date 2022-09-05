#include <gtest/gtest.h>

#include <trc_util/async/ThreadPool.h>

using namespace trc::async;

TEST(ThreadPoolTest, EmptyPool)
{
    ThreadPool pool(0);
    ASSERT_THROW(pool.async([]{}), std::invalid_argument);
}

TEST(ThreadPoolTest, ExecuteFunction)
{
    ThreadPool pool(4);
    auto fut = pool.async([]{ return 42; });
    ASSERT_EQ(fut.get(), 42);
}
