#pragma once

#include <vector>

#include "DrawInfo.h"

namespace trc::ui
{
    class DrawList
    {
    public:
        using iterator = std::vector<DrawInfo>::iterator;

        bool empty() const;

        void push_back(DrawInfo draw);
        void clear();

        auto begin() -> iterator;
        auto end() -> iterator;

    private:
        std::vector<DrawInfo> items;
    };
} // namespace trc::ui
