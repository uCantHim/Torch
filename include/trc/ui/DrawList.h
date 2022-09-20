#pragma once

#include <utility>
#include <vector>

#include "DrawInfo.h"

namespace trc::ui
{
    class Element;

    class DrawList
    {
    public:
        struct ScissorRect
        {
            ivec2 origin;
            ivec2 size;
        };

        struct Group
        {
            explicit Group(ScissorRect scissor) : scissorRect(scissor) {}

            bool empty() const {
                return lines.empty() && quads.empty() && texts.empty();
            }

            std::vector<DrawInfo<types::Quad>> quads;
            std::vector<DrawInfo<types::Line>> lines;
            std::vector<DrawInfo<types::Text>> texts;
            ScissorRect scissorRect;
        };

        using iterator = std::vector<Group>::iterator;
        using const_iterator = std::vector<Group>::const_iterator;

        explicit DrawList(const Window& window);

        bool empty() const;

        void push(types::Line prim, Element& el);
        void push(types::Quad prim, Element& el);
        void push(types::Text prim, Element& el);

        void pushScissorRect(Vec2D<pix_or_norm> origin, Vec2D<pix_or_norm> size);
        void popScissorRect();

        void clear();

        auto begin() -> iterator;
        auto end() -> iterator;
        auto begin() const -> const_iterator;
        auto end() const -> const_iterator;

    private:
        const Window& window;

        std::vector<Group> drawGroups;
    };
} // namespace trc::ui
