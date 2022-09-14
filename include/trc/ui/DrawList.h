#pragma once

#include <vector>

#include "DrawInfo.h"

namespace trc::ui
{
    class Element;

    class DrawList
    {
    public:
        using iterator = std::vector<DrawInfo>::iterator;

        bool empty() const;

        void push(types::Line prim, Element& el);
        void push(types::Quad prim, Element& el);
        void push(types::Text prim, Element& el);

        void push_back(DrawInfo draw);
        void clear();

        auto begin() -> iterator;
        auto end() -> iterator;

    private:
        std::vector<DrawInfo> items;
    };
} // namespace trc::ui
