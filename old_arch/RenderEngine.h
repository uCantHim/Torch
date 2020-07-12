#pragma once
#ifndef RENDERENGINE_H
#define RENDERENGINE_H

#include <memory>
#include <cstdint>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <thread>
#include <future>

#include "vkb/VulkanBase.h"
#include "vkb/FrameSpecificObject.h"

class VulkanDrawableInterface;
class Renderpass;
class Scene;

/**
 * @brief Asynchronously collects command lists
 */
class CommandCollector
{
public:
    using draw_iter = std::vector<VulkanDrawableInterface*>::iterator;

    CommandCollector();

    auto collect(const Renderpass& renderpass, draw_iter begin, draw_iter end)
        -> std::future<vk::CommandBuffer>;

    auto terminate() noexcept -> std::future<bool>;

private:
    void run();
    bool shouldTerminate{ false };
    std::promise<bool> isTerminated;

    struct CollectCommand
    {
        const Renderpass* renderpass;
        draw_iter begin;
        draw_iter end;
        std::promise<vk::CommandBuffer> result;
    };
    std::queue<CollectCommand> commandQueue;

    vkb::FrameSpecificObject<vk::UniqueCommandBuffer> primaryCommandBuffer;
};

/*
An engine that renders a specific Renderpass. */
class RenderEngine
{
public:
    explicit RenderEngine(const Renderpass& renderpass);
    ~RenderEngine();

    auto recordScene(Scene& scene) -> std::vector<vk::CommandBuffer>;

private:
    static constexpr size_t COMMAND_COLLECTOR_COUNT = 1;

    const Renderpass& renderpass;
    std::vector<CommandCollector> collectors;
};



#endif
