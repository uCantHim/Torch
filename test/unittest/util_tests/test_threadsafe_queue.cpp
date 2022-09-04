#include <atomic>
#include <future>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>
#include <trc_util/data/ThreadsafeQueue.h>

using namespace trc::data;

TEST(ThreadsafeQueueTest, EmptyAfterConstruct)
{
    ThreadsafeQueue<std::string> queue;

    ASSERT_EQ(queue.size(), 0);
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadsafeQueueTest, Push)
{
    ThreadsafeQueue<int> queue;
    for (int i = 0; i < 100; ++i) {
        queue.push(i * 2);
    }

    std::queue<int> truth;
    for (int i = 0; i < 100; ++i) {
        truth.push(i * 2);
    }

    ASSERT_EQ(queue, truth);
    ASSERT_EQ(queue.size(), 100);
    ASSERT_FALSE(queue.empty());
}

TEST(ThreadsafeQueueTest, Pop)
{
    ThreadsafeQueue<int> queue;
    std::queue<int> truth;
    for (int i = 0; i < 100; ++i)
    {
        queue.push(i * 4);
        truth.push(i * 4);
    }

    queue.wait_pop();
    ASSERT_NE(queue, truth);
    truth.pop();
    ASSERT_EQ(queue, truth);

    for (int i = 0; i < 52; ++i) {
        queue.wait_pop();
    }
    for (int i = 0; i < 52; ++i) {
        truth.pop();
    }
    ASSERT_EQ(queue, truth);

    ASSERT_EQ(queue.size(), 47);
    ASSERT_FALSE(queue.empty());
}

TEST(ThreadsafeQueueTest, ConstructFromContainer)
{
    std::vector<std::string> vec{ "hello", " ", "world", "!" };

    std::queue<std::string, std::vector<std::string>> truth(vec);
    ThreadsafeQueue<std::string, std::vector<std::string>> queue0(vec);
    ThreadsafeQueue<std::string, std::vector<std::string>> queue1(std::move(vec));
    ThreadsafeQueue<std::string, std::vector<std::string>> queue2;
    queue2.emplace("hello");
    queue2.emplace(" ");
    queue2.emplace("world");
    queue2.emplace("!");

    ASSERT_EQ(queue0, queue1);
    ASSERT_EQ(queue1, queue2);
    ASSERT_EQ(queue0, truth);
}

TEST(ThreadsafeQueueTest, WaitPop)
{
    ThreadsafeQueue<int> queue;
    auto future = std::async([&queue]{
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        queue.push(42);
    });

    const int i = queue.wait_pop();
    future.wait();

    ASSERT_EQ(i, 42);
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadsafeQueueTest, TryPop)
{
    ThreadsafeQueue<int> queue;
    auto future = std::async([&queue]{
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        queue.push(42);
    });

    ASSERT_FALSE(queue.try_pop());
    future.wait();
    auto val = queue.try_pop();

    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(val.value(), 42);
    ASSERT_TRUE(queue.empty());
}

TEST(ThreadsafeQueueTest, MultithreadedPushPop)
{
    constexpr int N = 500;

    ThreadsafeQueue<std::pair<int, std::string>> queue;
    std::atomic<int> counts[N]{ 0 };

    std::vector<std::thread> threads;
    threads.reserve(N * 2);
    for (int i = 0; i < N; ++i)
    {
        threads.emplace_back([&]{
            auto [index, str] = queue.wait_pop();
            ++counts[index];
        });
        threads.emplace_back([i, &queue]{ queue.emplace(i, "thread #" + std::to_string(i)); });
    }
    for (auto& t : threads) t.join();

    ASSERT_TRUE(queue.empty());
    for (int i = 0; i < N; ++i) {
        ASSERT_EQ(counts[i], 1);
    }
}

TEST(ThreadsafeQueueTest, RangeOperations)
{
    std::vector<std::string> vec{ "hello", " ", "world", "!" };
    // Push multiple items from an iterator range [first, last)
    ThreadsafeQueue<std::string> queue0;
    queue0.push(vec.begin(), vec.end());
    // Push multiple items from a range-type
    ThreadsafeQueue<std::string> queue1;
    queue1.push(vec);
    // Construct a queue from a range
    ThreadsafeQueue<std::string> queue2(vec);

    std::queue<std::string> truth;
    truth.emplace("hello");
    truth.emplace(" ");
    truth.emplace("world");
    truth.emplace("!");

    ASSERT_EQ(queue0, queue1);
    ASSERT_EQ(queue1, queue2);
    ASSERT_EQ(queue0, truth);
}

TEST(ThreadsafeQueueTest, MoveOperations)
{
    ThreadsafeQueue<int> queue;
    queue.push(42);

    ThreadsafeQueue<int> queue1(std::move(queue));
    ASSERT_TRUE(queue.empty());
    ASSERT_FALSE(queue.try_pop());
    ASSERT_EQ(queue1.size(), 1);
    ASSERT_TRUE(queue1.try_pop());

    queue1.push(std::vector<int>{ 0, 1, 2, 3, 4 });
    queue = std::move(queue1);
    ASSERT_TRUE(queue1.empty());
    ASSERT_FALSE(queue1.try_pop());
    ASSERT_EQ(queue.size(), 5);
    ASSERT_TRUE(queue.try_pop());
}
