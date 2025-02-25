#include "viewport/ViewportTree.h"

#include <cassert>



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
    struct Resize
    {
        void operator()(_Split& split)
        {
            const auto [fst, snd] = splitArea(area, split->split);
            std::visit(Resize{ fst }, split->first);
            std::visit(Resize{ snd }, split->second);
        }

        void operator()(_Leaf& leaf) {
            leaf->resize({ area.pos + kViewportPadding, area.size - 2u * kViewportPadding });
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

void ViewportTree::notify(const UserInput& input)
{
    if (auto target = findAt(cursorPos)) {
        target->notify(input);
    }
}

void ViewportTree::notify(const Scroll& scroll)
{
    if (auto target = findAt(cursorPos)) {
        target->notify(scroll);
    }
}

void ViewportTree::notify(const CursorMovement& cursorMove)
{
    cursorPos = cursorMove.position;

    if (auto target = findAt(cursorPos))
    {
        const auto [vpPos, vpSize] = target->getSize();
        CursorMovement cursor{
            .position = cursorMove.position - vec2{vpPos},
            .offset   = cursorMove.offset,
            .areaSize = vpSize,
        };
        target->notify(cursor);
    }
}

auto ViewportTree::findAt(ivec2 pos) -> Viewport*
{
    /**
     * @brief Visitor that finds the viewport that contains a point.
     */
    struct Finder
    {
        auto operator()(_Split& split) -> Viewport*
        {
            if (auto child = std::visit(*this, split->first)) {
                return child;
            }
            return std::visit(*this, split->second);
        }

        auto operator()(_Leaf& leaf) -> Viewport*
        {
            if (isInside(p, leaf->getSize())) {
                return leaf.get();
            }
            return nullptr;
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

auto ViewportTree::createSplit(
    Viewport* vp,
    const SplitInfo& split,
    s_ptr<Viewport> newVp,
    ViewportLocation newVpLoc)
    -> Viewport*
{
    if (Node* node = findNode(vp))
    {
        assert(std::holds_alternative<_Leaf>(*node));

        auto res = newVp.get();
        _Leaf curVp = std::move(std::get<_Leaf>(*node));
        // The node is not a valid unique_ptr anymore!

        *node = std::make_unique<Split>(Split{
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
    if (area) {
        vp->resize(*area);
    }
    floatingViewports.emplace_back(std::move(vp));
}

void ViewportTree::remove(Viewport* vp)
{
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
