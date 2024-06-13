#pragma once

#include <vector>

#include <componentlib/Table.h>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"
#include "trc/base/Device.h"
#include "trc/core/RenderStage.h"
#include "trc/core/DataFlow.h"

namespace trc
{
    class Frame;
    class RenderConfig;
    class SceneBase;
    class ResourceStorage;

    struct TaskEnvironment : DependencyRegion
    {
        const Device& device;

        Frame* frame;
        ResourceStorage* resources;
        SceneBase* scene;
    };

    class Task
    {
    public:
        virtual ~Task() noexcept = default;
        virtual void record(vk::CommandBuffer cmdBuf, TaskEnvironment& env) = 0;
    };

    /**
     * @brief Create a task from a function
     */
    template<std::invocable<vk::CommandBuffer, TaskEnvironment&> F>
    auto makeTask(F&& func) -> u_ptr<Task>;

    class TaskQueue
    {
    public:
        void spawnTask(RenderStage::ID stage, u_ptr<Task> task)
        {
            auto [taskList, _] = tasks.try_emplace(stage);
            taskList.emplace_back(std::move(task));
        }

        componentlib::Table<std::vector<u_ptr<Task>>, RenderStage::ID> tasks;
    };



    ///////////////////////////
    //    Implementations    //
    ///////////////////////////

    template<std::invocable<vk::CommandBuffer, TaskEnvironment&> F>
    auto makeTask(F&& func) -> u_ptr<Task>
    {
        struct FunctionalTask : public Task
        {
            FunctionalTask(const FunctionalTask&) = delete;
            FunctionalTask(FunctionalTask&&) noexcept = delete;
            FunctionalTask& operator=(const FunctionalTask&) = delete;
            FunctionalTask& operator=(FunctionalTask&&) noexcept = delete;

            FunctionalTask(F&& func) : func(std::forward<F>(func)) {}
            ~FunctionalTask() noexcept = default;

            void record(vk::CommandBuffer cmdBuf, TaskEnvironment& env) override {
                func(cmdBuf, env);
            }
            F func;
        };

        return std::make_unique<FunctionalTask>(std::forward<F>(func));
    }
} // namespace trc
