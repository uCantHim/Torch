#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>

#include <gtest/gtest.h>

#include <trc/base/Logging.h>
#include <trc/base/event/EventHandler.h>
#include <trc_util/async/ThreadPool.h>

using namespace trc;

// The standard guarantees that this works:
// https://stackoverflow.com/questions/6240950/platform-independent-dev-null-in-c
std::ofstream null(nullptr);

class EventHandlerTest : public testing::Test
{
protected:
    EventHandlerTest()
    {
        log::info.setOutputStream(null);
        EventThread::start();
    }

    ~EventHandlerTest()
    {
        EventThread::terminate();
    }

    template<typename T>
    void addListener(std::function<void(const T&)> func)
    {
        typename EventHandler<T>::ListenerId l = EventHandler<T>::addListener(func);
        listeners.emplace_back(std::unique_ptr<uint64_t, void(*)(uint64_t*)>(
            (uint64_t*)l,
            [](uint64_t* l) {
                EventHandler<T>::removeListener((typename EventHandler<T>::ListenerId)l);
            }
        ));
    }

private:
    std::vector<std::unique_ptr<uint64_t, void(*)(uint64_t*)>> listeners;
};

TEST_F(EventHandlerTest, EventThreadStartStop)
{
    EventThread::terminate();

    // Starting/stopping multiple times is still good
    EventThread::start();
    EventThread::start();
    EventThread::start();
    EventThread::terminate();
    EventThread::terminate();
    EventThread::terminate();
}

TEST_F(EventHandlerTest, SimpleEvent)
{
    int count{ 0 };
    int sum{ 111 };
    std::atomic<bool> signal{ false };

    addListener<int>([&](int i){
        while (!signal);  // Wait for the signal
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        sum += i;
        ++count;
    });

    EventHandler<int>::notify(256);
    EventHandler<int>::notify(0);
    EventHandler<int>::notify(-30);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_EQ(count, 0);
    ASSERT_EQ(sum, 111);

    // Signal that listeners may proceed
    signal = true;
    while (count != 3);

    ASSERT_EQ(count, 3);
    ASSERT_EQ(sum, 111 + 256 + 0 - 30);
}

TEST_F(EventHandlerTest, EventsFromMultipleThreads)
{
    const size_t kNumThreads{ std::thread::hardware_concurrency() * 4 };
    constexpr size_t kNumIterations{ 20 };
    constexpr int iVal{ 4 };
    constexpr float fVal{ -3.5f };

    std::atomic<int> count{ 0 };
    int sum{ 0 };
    float fSum{ 0 };
    addListener<int>([&](int i){ sum += i; ++count; });
    addListener<float>([&](float f){ fSum += f; ++count; });

    std::vector<std::future<void>> futures;
    futures.reserve(kNumThreads);
    for (size_t i = 0; i < kNumThreads; ++i)
    {
        futures.emplace_back(std::async([index=i]{
            std::this_thread::sleep_for(std::chrono::microseconds(15));
            for (size_t i = 0; i < kNumIterations; ++i)
            {
                if (index % 2 == 0) {
                    EventHandler<int>::notify(iVal);
                }
                else {
                    EventHandler<float>::notify(fVal);
                }
            }
        }));
    }

    for (auto& f : futures) f.wait();
    while (count != kNumThreads * kNumIterations);  // Wait for all events to be processed

    ASSERT_EQ(count, kNumThreads * kNumIterations);
    ASSERT_EQ(sum, iVal * (kNumThreads / 2) * kNumIterations);
    ASSERT_FLOAT_EQ(fSum, fVal * static_cast<float>(kNumThreads / 2) * kNumIterations);
}
