#include "viewport/ViewportTree.h"

#include <cassert>
#include <iostream>
#include <ranges>

#include <trc_util/Assert.h>



bool ViewportTree::SplitLine::isHorizontal() const
{
    if (auto s = self.lock()) {
        return s->split.horizontal;
    }
    return false;
}

bool ViewportTree::SplitLine::isVertical() const
{
    if (auto s = self.lock()) {
        return !s->split.horizontal;
    }
    return false;
}

auto ViewportTree::SplitLine::getSplitArea() const -> ViewportArea
{
    if (auto s = self.lock()) {
        return s->area;
    }
    return {};
}

auto ViewportTree::SplitLine::getLocation() const -> ui32
{
    if (auto s = self.lock())
    {
        /**
         * We have to subtract kViewportPadding from the split location because
         * the size of the split area is adjusted during layout calculations to
         * incorporate the padding between the child elements, which is always
         * `kViewportPadding * 2`. `getPosInPixels` now calculates the split
         * location with respect to this new size, offsetting it from its true
         * location.
         */

        const ui32 parentSize = s->split.horizontal ? s->area.size.y : s->area.size.x;
        return s->split.location.getPosInPixels(parentSize) - kViewportPadding;
    }
    return {};
}

void ViewportTree::SplitLine::moveBy(i32 pixels)
{
    if (auto s = self.lock())
    {
        const ui32 parentSize = s->split.horizontal ? s->area.size.y : s->area.size.x;
        auto loc = s->split.location.getPosInPixels(parentSize);
        s->split.location = SplitLocation::makePixel(loc + pixels);

        tree->resize(tree->getSize());
    }
}

void ViewportTree::SplitLine::setLocation(SplitLocation loc)
{
    if (auto s = self.lock())
    {
        s->split.location = loc;
        tree->resize(tree->getSize());
    }
}

void ViewportTree::SplitLine::setFirstChild(s_ptr<Viewport> newChild)
{
    if (auto s = self.lock())
    {
        if (newChild != nullptr) {
            s->first = newChild;
        }
    }
}

void ViewportTree::SplitLine::setSecondChild(s_ptr<Viewport> newChild)
{
    if (auto s = self.lock())
    {
        if (newChild != nullptr) {
            s->second = newChild;
        }
    }
}

void ViewportTree::SplitLine::removeChild(ViewportLocation which)
{
    if (auto s = self.lock()) {
        tree->mergeSplit(s.get(), which);
    }
}



struct ViewportTree::PrintTree
{
    void operator()(_Split& split)
    {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "Split: ";
        printArea(split->area);
        std::cout << "\n";

        std::visit(PrintTree{ indent + 2 }, split->first);
        std::visit(PrintTree{ indent + 2 }, split->second);
    }

    void operator()(_Leaf& leaf)
    {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "Leaf: ";
        printArea(leaf->getSize());
        std::cout << "\n";
    }

    static void printArea(const ViewportArea& area)
    {
        std::cout << "(" << area.pos.x << ", " << area.pos.y << ") - ("
                  << area.size.x << ", " << area.size.y << ")";
    }

    const int indent{ 0 };
};



bool operator==(const ViewportTree::Node& node, const Viewport* vp)
{
    return std::visit(trc::util::VariantVisitor{
        [](const ViewportTree::_Split&){ return false; },
        [vp](const ViewportTree::_Leaf& leaf){ return leaf.get() == vp; },
    }, node);
}

bool operator==(const ViewportTree::Node& node, const ViewportTree::Split* split)
{
    return std::visit(trc::util::VariantVisitor{
        [split](const ViewportTree::_Split& s){ return s.get() == split; },
        [](const ViewportTree::_Leaf&){ return false; },
    }, node);
}

bool ViewportTree::isInside(ivec2 p, const ViewportArea& area)
{
    const auto [pos, size] = area;
    return glm::all(glm::lessThanEqual(pos, p))
        && glm::all(glm::lessThan(p, pos + ivec2{size}));
}



ViewportTree::ViewportTree(const ViewportArea& size, s_ptr<Viewport> rootVp)
    :
    root(_Leaf{nullptr})
{
    assert_arg(rootVp != nullptr);
    root = std::move(rootVp);
    resize(size);
}

void ViewportTree::draw(trc::Frame& frame)
{
    struct DrawViewports
    {
        void operator()(_Split& split)
        {
            std::visit(*this, split->first);
            std::visit(*this, split->second);
        }

        void operator()(_Leaf& leaf) {
            leaf->draw(frame);
        }

        trc::Frame& frame;
    };

    // Draw the tree first
    std::visit(DrawViewports{ frame }, root);

    // Now draw all floating viewports
    for (auto& vp : floatingViewports) {
        vp->draw(frame);
    }
}

void ViewportTree::resize(const ViewportArea& newArea)
{
    // Reposition floating viewports such that they have the same relative
    // position to the origin on the new window area.
    std::vector<vec2> relativePositions;
    for (const vec2 size = this->getSize().size;
         auto& vp : floatingViewports)
    {
        const auto [vpPos, vpSize] = vp->getSize();
        const vec2 relativePos = vec2{vpPos} / size;
        vp->resize({ relativePos * vec2{newArea.size}, vpSize });
    }

    // Resize viewports in the tree.
    viewportArea = newArea;
    resizeElem(root, newArea);
}

auto ViewportTree::getSize() -> ViewportArea
{
    return viewportArea;
}

auto ViewportTree::findViewportAt(ivec2 pos) -> Viewport*
{
    if (auto el = findElemAt(pos)) {
        return std::holds_alternative<_Leaf>(*el) ? std::get<_Leaf>(*el).get() : nullptr;
    }
    return nullptr;
}

auto ViewportTree::findAt(ivec2 pos) -> std::optional<std::variant<Viewport*, SplitLine>>
{
    using Res = std::variant<Viewport*, SplitLine>;
    if (auto el = findElemAt(pos))
    {
        return std::visit(trc::util::VariantVisitor{
            [](_Leaf& leaf) -> Res { return leaf.get(); },
            [&](_Split& split) -> Res { return SplitLine{ split, this }; },
        }, *el);
    }

    return std::nullopt;
}

auto ViewportTree::traverse() -> std::generator<std::variant<Viewport*, SplitLine>>
{
    struct Traverser
    {
        auto operator()(_Split& split) -> std::generator<std::variant<Viewport*, SplitLine>>
        {
            co_yield SplitLine{ split, tree };
            co_yield std::ranges::elements_of(std::visit(*this, split->first));
            co_yield std::ranges::elements_of(std::visit(*this, split->second));
        }

        auto operator()(_Leaf& leaf) -> std::generator<std::variant<Viewport*, SplitLine>>
        {
            co_yield leaf.get();
        }

        ViewportTree* tree;
    };

    co_yield std::ranges::elements_of(std::visit(Traverser{ this }, root));
}

auto ViewportTree::createSplit(
    Viewport* vp,
    const SplitInfo& split,
    s_ptr<Viewport> newVp,
    ViewportLocation newVpLoc)
    -> std::optional<SplitLine>
{
    if (vp == nullptr || newVp == nullptr) {
        return std::nullopt;
    }

    if (Node* node = findNode(vp))
    {
        assert(std::holds_alternative<_Leaf>(*node));

        _Leaf curVp = std::move(std::get<_Leaf>(*node));
        *node = std::make_unique<Split>(Split{
            .area   = vp->getSize(),
            .split  = split,
            .first  = newVpLoc == ViewportLocation::eFirst ? newVp : curVp,
            .second = newVpLoc == ViewportLocation::eFirst ? curVp : newVp,
        });

        // Recalculate tree layout
        resizeElem(*node, vp->getSize());

        return SplitLine{ std::get<_Split>(*node), this };
    }

    return std::nullopt;
}

void ViewportTree::createFloating(s_ptr<Viewport> vp, std::optional<ViewportArea> area)
{
    if (vp != nullptr)
    {
        if (area) {
            vp->resize(*area);
        }
        floatingViewports.emplace_back(std::move(vp));
    }
}

void ViewportTree::remove(Viewport* vp)
{
    if (vp == nullptr) {
        return;
    }

    // Test whether the viewport is floating
    auto it = std::ranges::find_if(floatingViewports, [&](auto& el){ return vp == el.get(); });
    if (it != floatingViewports.end())
    {
        floatingViewports.erase(it);
        return;
    }

    // Viewport is not floating, try to remove it from the tree
    if (auto parent = findParent(vp))
    {
        assert(vp == parent->first || vp == parent->second);
        mergeSplit(
            parent,
            vp == parent->first ? ViewportLocation::eFirst
                                : ViewportLocation::eSecond
        );
    }
}

auto ViewportTree::findParent(std::variant<Split*, Viewport*> elem) -> Split*
{
    return std::visit([this](auto&& el){
        return std::visit(FindParent{ el }, root);
    }, elem);
}

auto ViewportTree::findNode(std::variant<Split*, Viewport*> elem) -> Node*
{
    // Treat the special case where the node cannot be found through the
    // element's parent because the element is the root node.
    auto isRoot = [this](auto&& elem){ return root == elem; };
    if (std::visit(isRoot, elem)) {
        return &root;
    }

    // Find the element's node by looking it up in the element's parent.
    return std::visit(trc::util::VariantVisitor{
        [this](auto&& elem) -> Node* {
            if (auto parent = findParent(elem))
            {
                assert(parent->first == elem || parent->second == elem);
                return parent->first == elem ? &parent->first : &parent->second;
            }
            return nullptr;
        },
    }, elem);
}

void ViewportTree::mergeSplit(Split* split, ViewportLocation removedViewport)
{
    assert(split != nullptr);
    if (auto node = findNode(split))
    {
        *node = removedViewport == ViewportLocation::eFirst
            ? std::move(split->second)
            : std::move(split->first);

        // Resize the parent because a split's area might not be appropriate if
        // the child which replaces it is a viewport.
        auto parent = findParent(split);
        resizeElem(*findNode(parent), parent->area);
    }
}

void ViewportTree::resizeElem(Node& elem, const ViewportArea& newArea)
{
    /**
     * The visitor returns the visited child's new size. We do this to be able
     * to calculate sizes of split nodes correctly - this is sort of a bi-
     * -directional tree traversal approach.
     *
     * Other methods have trouble with the padding because it has to be applied
     * per viewport, not per nesting level.
     */
    struct Resize
    {
        auto operator()(_Split& split) -> ViewportArea
        {
            auto [fst, snd] = splitArea(area, split->split);
            fst = std::visit(Resize{ fst }, split->first);
            snd = std::visit(Resize{ snd }, split->second);

            // Note that only the width of the split line must be included, not
            // the padding on all sides of the children; Padding is only applied
            // at the leaf level to avoid multiplying it when nesting.
            ViewportArea size;
            if (split->split.horizontal) {
                size = { fst.pos, { fst.size.x, snd.size.y + fst.size.y + kViewportPadding * 2 } };
            }
            else /* if split.vertical */ {
                size = { fst.pos, { fst.size.x + snd.size.x + kViewportPadding * 2, fst.size.y } };
            }

            split->area = size;
            return size;
        }

        auto operator()(_Leaf& leaf) -> ViewportArea
        {
            leaf->resize({ area.pos + kViewportPadding, area.size - 2u * kViewportPadding });
            return leaf->getSize();
        }

        const ViewportArea area;
    };

    // Resize viewports in the tree.
    std::visit(Resize{ newArea }, elem);
}

auto ViewportTree::getElemSize(const Node& node) -> ViewportArea
{
    return std::visit(trc::util::VariantVisitor{
        [](const _Split& split){ return split->area; },
        [](const _Leaf& leaf){ return leaf->getSize(); },
    }, node);
}

auto ViewportTree::findElemAt(ivec2 pos) -> std::optional<std::variant<_Split, _Leaf>>
{
    /**
     * @brief Visitor that finds the entity at a position.
     */
    struct Finder
    {
        auto operator()(_Split& split) -> std::optional<std::variant<_Split, _Leaf>>
        {
            // Test whether the point is anywhere in the split
            if (!isInside(p, split->area)) {
                return std::nullopt;
            }

            // Test the children
            if (auto child = std::visit(*this, split->first)) {
                return child;
            }
            if (auto child = std::visit(*this, split->second)) {
                return child;
            }

            // Now test whether the point is on the split line, i.e., exactly
            // between the children.
            const auto fst = getElemSize(split->first);
            const auto snd = getElemSize(split->second);

            const vec2 axisMap = split->split.horizontal ? vec2{ 0, 1 } : vec2{ 1, 0 };
            const float axisPos = glm::dot(vec2{p}, axisMap);
            if (glm::dot(vec2{fst.pos} + vec2{fst.size}, axisMap) < axisPos
                && axisPos < glm::dot(vec2{snd.pos}, axisMap))
            {
                return split;
            }

            // Point is not anywhere in the split area
            return std::nullopt;
        }

        auto operator()(_Leaf& leaf) -> std::optional<std::variant<_Split, _Leaf>>
        {
            if (isInside(p, leaf->getSize())) {
                return leaf;
            }
            return std::nullopt;
        }

        const ivec2 p;
    };

    // Search floating viewports first.
    // Viewports at the end of the list are considered 'in front of' viewports
    // at the beginning.
    for (auto& vp : std::views::reverse(floatingViewports))
    {
        if (isInside(pos, vp->getSize())) {
            return vp;
        }
    }

    // Now search the viewport tree.
    return std::visit(Finder{ pos }, root);
}
