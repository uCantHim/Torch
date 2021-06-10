#pragma once

#include "ThreadPool.h"

namespace nc::async
{
    template<typename Func, typename... Args>
    inline auto async(Func func, Args&&... args)
    {
        static ThreadPool threadPool;

        return threadPool.async(std::move(func), std::forward<Args>(args)...);
    }
} // namespace nc::async
