#include <chrono>
#include <iostream>

using namespace std::chrono;

#include <vkb/Buffer.h>
#include <vkb/Device.h>
#include <vkb/MemoryPool.h>
#include <vkb/Swapchain.h>
#include <vkb/VulkanBase.h>
#include <vkb/VulkanInstance.h>

void runTest(const vkb::Device& device, size_t numBuffers, size_t memoryPerBuffer)
{
    std::vector<vkb::Buffer> buffers;
    buffers.reserve(numBuffers);

    auto memStart = system_clock::now();
    for (size_t i = 0; i < numBuffers; i++)
    {
        buffers.emplace_back(device, memoryPerBuffer,
                             vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlags{});
    }
    auto memEnd = system_clock::now();

    buffers.clear();
    vkb::MemoryPool pool(device, numBuffers * memoryPerBuffer);

    auto poolStart = system_clock::now();
    for (size_t i = 0; i < numBuffers; i++)
    {
        buffers.emplace_back(device, memoryPerBuffer,
                             vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlags{},
                             pool.makeAllocator());
    }
    auto poolEnd = system_clock::now();

    std::cout << "Single allocation per buffer: "
        << duration_cast<microseconds>(memEnd - memStart).count() << " µs\n";
    std::cout << "Allocation from memory pool:  "
        << duration_cast<microseconds>(poolEnd - poolStart).count() << " µs\n";
}

int main()
{
    vkb::init();
    vkb::VulkanInstance instance;
    auto surface = vkb::makeSurface(*instance, {});
    vkb::Device device(vkb::device_helpers::getOptimalPhysicalDevice(*instance, *surface.surface));

    constexpr size_t memory{ 2000000000 };  // 2 GB

    for (size_t i = 0; i < 6; i++)
    {
        const size_t numBuffers{ 2 * static_cast<size_t>(std::pow(10, i)) };
        const size_t memoryPerBuffer{ memory / numBuffers };

        std::cout << numBuffers << " buffers with " << memoryPerBuffer << " memory each:\n";
        runTest(device, numBuffers, memoryPerBuffer);
        std::cout << "\n";
    }

    vkb::terminate();

    return 0;
}
