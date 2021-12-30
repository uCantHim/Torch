#pragma once

#include "ThreadPool.h"

namespace trc::async
{
    template<typename Func, typename... Args>
    inline auto async(Func func, Args&&... args)
    {
        static ThreadPool threadPool;

        return threadPool.async(std::move(func), std::forward<Args>(args)...);
    }
} // namespace trc::async
