#pragma once

#include <variant>

#include "viewport/Viewport.h"
#include "viewport/Split.h"

enum class ViewportLocation
{
    eFirst, eSecond
};

class ViewportTree : public Viewport
{
public:
    ViewportTree(const ViewportArea& size, s_ptr<Viewport> rootVp);

    void draw(trc::Frame& frame) override;

    void resize(const ViewportArea& newArea) override;
    auto getSize() -> ViewportArea override;

    void notify(const UserInput& input) override;
    void notify(const Scroll& scroll) override;
    void notify(const CursorMovement& cursorMove) override;

    auto findAt(ivec2 pos) -> Viewport*;

    /**
     * Insert a viewport into the tree by splitting an existing viewport.
     */
    auto createSplit(Viewport* vp,
                     const SplitInfo& split,
                     s_ptr<Viewport> newViewport,
                     ViewportLocation newViewportLocation)
        -> Viewport*;

    /**
     * Merges the viewport's parent.
     */
    void remove(Viewport* viewport);

    // void createFloating(s_ptr<Viewport> vp);

private:
    /**
     * A small essay on the thoughts around this tree architecture
     *
     * I thought about the traditional 'object-oriented' (read: class- and
     * inheritance-oriented) approach. In this scenario, we would have a class
     * `SplitViewport : Viewport` that represents nodes. Its implementations of
     * the `notify*` functions, for example, would forward events transparently
     * to their respective children, performing bounding box checking and so
     * forth. Everything is a viewport and can be treated as such, which makes
     * the components flexible.
     *
     * This sounds and looks extremely clean. It's a perfect application of the
     * class-oriented paradigm.
     *
     * What you get from this approach is a clean tree structure where each node
     * is responsible to manage itself and knowns nothing about any other node.
     * However, the structure is also completely opaque from the outside. To
     * split a viewport into two, one has to obtain the viewport's parent and
     * access its children (remove the current viewport as a child and insert a
     * new viewport, namely a `SplitViewport` in its place). In other words, the
     * parent must be treated as a *split viewport*, not as the generic viewport
     * that it appears to be as long as it lives in the tree. This breaks the
     * abstraction.
     *
     * Concretely, one could:
     *
     *    - Incorporate the possibility of splitting into the interface. This
     *    dilutes the abstraction, requires viewports to know about parents, ...
     *    Or one could build a separate private inheritance hierarchy that
     *    represents this without cluttering the public `Viewport` interface.
     *
     *    - Use `dynamic_cast` in the viewport tree's methods (note that until
     *    now, we didn't even need a viewport tree class, but now we need some
     *    kind of omniscient type-aware observer). This sounds like complete
     *    crap because it is. `dynamic_cast` means: I wanted to use this
     *    paradigm, but it doesn't work for my problem, so I need to circument
     *    it.
     *
     * As you can see, both methods naturally point toward a structure built
     * on strong sum types. So that's what I did here.
     */

    struct Split;
    using _Split = u_ptr<Split>;
    using _Leaf = s_ptr<Viewport>;
    using Node = std::variant<_Split, _Leaf>;

    struct Split
    {
        SplitInfo split;
        Node first;
        Node second;
    };

    friend bool operator==(const Node& node, const Viewport* vp);
    friend bool operator==(const Node& node, const Split* vp);

    /**
     * @brief Visitor that finds a tree element's parent.
     *
     * The parent is always a split.
     */
    template<typename T>
        requires std::same_as<T, Split*> || std::same_as<T, Viewport*>
    struct FindParent
    {
        auto operator()(_Split& split) -> Split*
        {
            if (isParent(split)) {
                return split.get();
            }

            // Test children
            if (auto parent = std::visit(*this, split->first)) {
                return parent;
            }
            return std::visit(*this, split->second);
        }

        auto operator()(const _Leaf&) -> Split* {
            return nullptr;
        }

        bool isParent(_Split& split) const {
            return elem == split->first || elem == split->second;
        };

        T elem;
    };

    auto findParent(std::variant<Split*, Viewport*> elem) -> Split*;
    auto findNode(std::variant<Split*, Viewport*> elem) -> Node*;
    void mergeSplit(Split* split, ViewportLocation removedViewport);

    Node root;

    // Total extent of the viewport tree.
    ViewportArea viewportArea;

    // The current cursor position in window coordinates.
    // Cached because we need it for all events to decide which viewport they
    // affect.
    ivec2 cursorPos{ 0, 0 };
};
