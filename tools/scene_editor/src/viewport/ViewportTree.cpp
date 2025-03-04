#include "viewport/ViewportTree.h"

#include <cassert>
#include <iostream>

#include <trc_util/Assert.h>



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
    root(_Leaf{nullptr}),
    viewportArea(size)
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
    std::visit(Resize{ newArea }, root);
}

auto ViewportTree::getSize() -> ViewportArea
{
    return viewportArea;
}

auto ViewportTree::notify(const UserInput& input) -> NotifyResult
{
    if (auto target = findAt(cursorPos)) {
        return target->notify(input);
    }
    return NotifyResult::eRejected;
}

auto ViewportTree::notify(const Scroll& scroll) -> NotifyResult
{
    if (auto target = findAt(cursorPos)) {
        return target->notify(scroll);
    }
    return NotifyResult::eRejected;
}

auto ViewportTree::notify(const CursorMovement& cursorMove) -> NotifyResult
{
    cursorPos = cursorMove.position;

    if (auto target = findAt(cursorPos))
    {
        const auto [vpPos, vpSize] = target->getSize();
        CursorMovement childCursorMove{
            .position = cursorMove.position - vec2{vpPos},
            .offset   = cursorMove.offset,
            .areaSize = vpSize,
        };

        return target->notify(childCursorMove);
    }
    return NotifyResult::eRejected;
}

auto ViewportTree::findAt(ivec2 pos) -> Viewport*
{
    if (auto el = findElemAt(pos)) {
        return std::holds_alternative<Viewport*>(*el) ? std::get<Viewport*>(*el) : nullptr;
    }
    return nullptr;
}

auto ViewportTree::createSplit(
    Viewport* vp,
    const SplitInfo& split,
    s_ptr<Viewport> newVp,
    ViewportLocation newVpLoc)
    -> Viewport*
{
    if (vp == nullptr || newVp == nullptr) {
        return nullptr;
    }

    if (Node* node = findNode(vp))
    {
        assert(std::holds_alternative<_Leaf>(*node));

        auto res = newVp.get();
        _Leaf curVp = std::move(std::get<_Leaf>(*node));
        // The node is not a valid unique_ptr anymore!

        *node = std::make_unique<Split>(Split{
            .area=vp->getSize(),
            .split=split,
            .first=newVpLoc == ViewportLocation::eFirst ? std::move(newVp) : std::move(curVp),
            .second=newVpLoc == ViewportLocation::eFirst ? std::move(curVp) : std::move(newVp),
        });

        // Recalculate tree layout
        resize(getSize());  // TODO: Use the Resize visitor from ViewportTree::resize

        return res;
    }

    return nullptr;
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

        // Recalculate tree layout
        resize(getSize());
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
    if (!(std::holds_alternative<_Leaf>(split->first)
          && std::holds_alternative<_Leaf>(split->second)))
    {
        throw std::logic_error("Merging splits with non-splits is not yet implemented.");
    }

    if (auto node = findNode(split))
    {
        *node = removedViewport == ViewportLocation::eFirst
            ? std::move(split->first)
            : std::move(split->second);
    }
}

auto ViewportTree::getElemSize(const Node& node) -> ViewportArea
{
    return std::visit(trc::util::VariantVisitor{
        [](const _Split& split){ return split->area; },
        [](const _Leaf& leaf){ return leaf->getSize(); },
    }, node);
}

auto ViewportTree::findElemAt(ivec2 pos) -> std::optional<std::variant<Viewport*, Split*>>
{
    /**
     * @brief Visitor that finds the entity at a position.
     */
    struct Finder
    {
        auto operator()(_Split& split) -> std::optional<std::variant<Viewport*, Split*>>
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
                return split.get();
            }

            // Point is not anywhere in the split area
            return std::nullopt;
        }

        auto operator()(_Leaf& leaf) -> std::optional<std::variant<Viewport*, Split*>>
        {
            if (isInside(p, leaf->getSize())) {
                return leaf.get();
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
            return vp.get();
        }
    }

    // Now search the viewport tree.
    return std::visit(Finder{ pos }, root);
}
