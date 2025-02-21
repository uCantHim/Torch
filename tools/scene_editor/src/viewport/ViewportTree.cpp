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

    std::visit(DrawViewports{ frame }, root);
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
            leaf->resize(area);
        }

        const ViewportArea area;
    };

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
            const auto [pos, size] = leaf->getSize();
            if (glm::all(glm::lessThanEqual(pos, p))
                && glm::all(glm::lessThan(p, pos + ivec2{size})))
            {
                return leaf.get();
            }
            return nullptr;
        }

        const ivec2 p;
    };

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

void ViewportTree::remove(Viewport* viewport)
{
    if (auto parent = findParent(viewport))
    {
        assert(viewport == parent->first || viewport == parent->second);
        mergeSplit(
            parent,
            viewport == parent->first ? ViewportLocation::eFirst
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
    assert(std::holds_alternative<_Leaf>(split->first));
    assert(std::holds_alternative<_Leaf>(split->second));

    if (auto node = findNode(split))
    {
        *node = removedViewport == ViewportLocation::eFirst
            ? std::move(split->first)
            : std::move(split->second);
    }
}
