#pragma once

#include <concepts>
#include <functional>
#include <generator>
#include <unordered_map>
#include <vector>

#include <trc/Types.h>
#include <trc/VulkanInclude.h>
#include <trc/core/RenderStage.h>

namespace trc::impl
{
    template<typename Context>
    class Task
    {
    public:
        virtual ~Task() noexcept = default;
        virtual void record(vk::CommandBuffer cmdBuf, Context& ctx) = 0;
    };

    template<typename Context>
    class TaskQueue
    {
    public:
        using TaskT = Task<Context>;

        TaskQueue() = default;

        void spawnTask(RenderStage::ID stage, u_ptr<TaskT> task);

        template<std::invocable<vk::CommandBuffer, Context&> F>
        void spawnTask(RenderStage::ID stage, F&& taskFunc) {
            spawnTask(stage, makeTask(std::forward<F>(taskFunc)));
        }

        auto iterTasks(RenderStage::ID stage) -> std::generator<TaskT&>;
        void clear();

        /**
         * Move the queue's tasks to another queue of a different context type.
         * The queue will be left empty.
         *
         * Note that this function packages all tasks of a render stage into
         * one single task in the destination queue.
         *
         * @param contextBuilder A function that transforms a context object of
         *                       the destination queue to a context object of
         *                       the source queue (the queue on which this
         *                       function is called). The resulting object will
         *                       be passed to the source queue's tasks.
         * @param dstQueue The queue to which this queue's tasks are moved.
         */
        template<typename OtherContext>
        void moveTasks(std::function<Context(OtherContext&)> contextBuilder,
                       TaskQueue<OtherContext>& dstQueue);

    private:
        template<std::invocable<vk::CommandBuffer, Context&> F>
        auto makeTask(F&& func) -> u_ptr<TaskT>;

        /**
         * Used to move tasks to other queues. See `TaskQueue<>::moveTasks`.
         */
        template<typename OtherContext>
        struct TaskWrapper
        {
            void operator()(vk::CommandBuffer cmdBuf, OtherContext& baseCtx)
            {
                auto ctx = contextBuilder(baseCtx);
                for (auto& task : tasks) {
                    task->record(cmdBuf, ctx);
                }
            }

            std::vector<u_ptr<TaskT>> tasks;
            std::function<Context(OtherContext&)> contextBuilder;
        };

        std::unordered_map<RenderStage::ID, std::vector<u_ptr<TaskT>>> taskList;
    };



    ///////////////////////////
    //    Implementations    //
    ///////////////////////////

    template<typename Context>
    inline void TaskQueue<Context>::spawnTask(RenderStage::ID stage, u_ptr<TaskT> task)
    {
        if (task != nullptr)
        {
            auto [tasks, _] = taskList.try_emplace(stage);
            tasks->second.emplace_back(std::move(task));
        }
    }

    template<typename Context>
    inline auto TaskQueue<Context>::iterTasks(RenderStage::ID stage)
        -> std::generator<TaskT&>
    {
        auto [it, _] = taskList.try_emplace(stage);
        for (auto& task : it->second) {
            co_yield *task;
        }
    }

    template<typename Context>
    template<typename OtherContext>
    inline void TaskQueue<Context>::moveTasks(
        std::function<Context(OtherContext&)> contextBuilder,
        TaskQueue<OtherContext>& dstQueue)
    {
        for (auto& [stage, tasks] : taskList)
        {
            // All task lists are empty after this. However, the map
            // [stage -> task_list] is not cleared to improve performance if the
            // task queue is reused.
            dstQueue.spawnTask(
                stage,
                TaskWrapper<OtherContext>{ std::move(tasks), contextBuilder }
            );
        }
    }

    template<typename Context>
    template<std::invocable<vk::CommandBuffer, Context&> F>
    inline auto TaskQueue<Context>::makeTask(F&& func) -> u_ptr<TaskT>
    {
        struct FunctionalTask : public TaskT
        {
            FunctionalTask(const FunctionalTask&) = delete;
            FunctionalTask(FunctionalTask&&) noexcept = delete;
            FunctionalTask& operator=(const FunctionalTask&) = delete;
            FunctionalTask& operator=(FunctionalTask&&) noexcept = delete;

            FunctionalTask(F&& func) : func(std::forward<F>(func)) {}
            ~FunctionalTask() noexcept = default;

            void record(vk::CommandBuffer cmdBuf, Context& ctx) override {
                func(cmdBuf, ctx);
            }
            F func;
        };

        return std::make_unique<FunctionalTask>(std::forward<F>(func));
    }
}
