#pragma once

#include <generator>
#include <optional>
#include <variant>
#include <vector>

#include "viewport/Split.h"
#include "viewport/Viewport.h"

enum class ViewportLocation
{
    eFirst, eSecond
};

/**
 * @brief A binary tree of tiled viewports.
 *
 * Forwards received input events to the viewport that is currently hovered by
 * the cursor.
 */
class ViewportTree
{
private:
    struct Split;

public:
    /**
     * @brief The border between the children of a split viewport.
     */
    class SplitLine
    {
    public:
        bool isHorizontal() const;
        bool isVertical() const;

        /**
         * @return Total size of the split area, including both children and
         *         the padding between them.
         */
        auto getSplitArea() const -> ViewportArea;

        /**
         * @return The location of the split line on the split axis in pixels.
         *         Is an offset relative to the split area's origin.
         */
        auto getLocation() const -> ui32;

        /**
         * @brief Move the split line by an amount of pixels on the split axis.
         */
        void moveBy(i32 pixels);

        /**
         * @brief Change the location of the split line on the split axis.
         *
         * @param loc The new split location. Interpreted as an offset from the
         *            split area's origin.
         */
        void setLocation(SplitLocation loc);

        void setFirstChild(s_ptr<Viewport> newChild);
        void setSecondChild(s_ptr<Viewport> newChild);

        /**
         * @brief Collapse the split into a single viewport.
         *
         * Remove one of the children and replace the split with the remaining
         * child.
         *
         * @param which A flag that indicates which child to remove.
         */
        void removeChild(ViewportLocation which);

    private:
        friend ViewportTree;
        SplitLine(const s_ptr<Split>& _split, ViewportTree* _parent)
            : self(_split), tree(_parent) {}

        w_ptr<Split> self;  // The tree has been destroyed if this is null.
        ViewportTree* tree;
    };

    /**
     * @param size   Initial size of the viewport tree's root.
     * @param rootVp An initial viewport that will cover the entire tree area.
     *               Required because a viewport tree cannot have empty viewport
     *               slots. Must not be `nullptr`.
     *
     * @throw std::invalid_argument if `rootVp` is `nullptr`.
     */
    ViewportTree(const ViewportArea& size, s_ptr<Viewport> rootVp);

    /**
     * @brief Draw the contents of all viewports in the tree.
     */
    void draw(trc::Frame& frame);

    void resize(const ViewportArea& newArea);
    auto getSize() -> ViewportArea;

    /**
     * @brief Try to find a viewport that encloses a specific point.
     *
     * Prefers floating viewports over static ones if the point intersects both.
     *
     * @param pos
     *
     * @return A viewport if one resides at `pos`. `nullptr` if `pos` does not
     *         touch any viewport.
     */
    auto findViewportAt(ivec2 pos) -> Viewport*;

    /**
     * @brief Try to find a tree element at a specific point.
     *
     * @return Either a viewport or a split line between two viewports. Nothing
     *         if the position is not in the viewport tree at all.
     */
    auto findAt(ivec2 pos) -> std::optional<std::variant<Viewport*, SplitLine>>;

    /**
     * @brief Traverse the tree.
     *
     * Visit all nodes in the viewport tree pre-order (root, first child, then
     * second child).
     */
    auto traverse() -> std::generator<std::variant<Viewport*, SplitLine>>;

    /**
     * @brief Insert a viewport into the tree by splitting an existing viewport.
     *
     * @param vp          The existing viewport to split into two. Must be part
     *                    of the tree.
     * @param split       A description of how to layout the split.
     * @param newViewport A viewport to insert into the tree. Must not be
     *                    `nullptr`.
     * @param newViewportLocation The new viewport's location within the split.
     *
     * @return A handle to the new split if the operation was successful.
     *         Nothing otherwise.
     */
    auto createSplit(Viewport* vp,
                     const SplitInfo& split,
                     s_ptr<Viewport> newViewport,
                     ViewportLocation newViewportLocation)
        -> std::optional<SplitLine>;

    /**
     * @brief Create a floating viewport.
     *
     * Floating viewports are not subject to tree layout based resizing and are
     * always considered to be 'in front of' the static viewport tree.
     *
     * Use `remove` to close floating viewports.
     *
     * @param vp   The new floating viewport. Passing `nullptr` will not create
     *             a new viewport.
     * @param area An optional initial position and size for the new viewport.
     */
    void createFloating(s_ptr<Viewport> vp, std::optional<ViewportArea> area = {});

    /**
     * @brief Remove a viewport from the tree.
     *
     * Removing a viewport that is part of a split closes that split and causes
     * the other half to occupy the combined area.
     *
     * @param viewport The viewport to remove from the tree.
     */
    void remove(Viewport* viewport);

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

    using _Split = s_ptr<Split>;
    using _Leaf = s_ptr<Viewport>;
    using Node = std::variant<_Split, _Leaf>;

    struct Split
    {
        ViewportArea area;

        SplitInfo split;
        Node first;
        Node second;
    };

    friend bool operator==(const Node& node, const Viewport* vp);
    friend bool operator==(const Node& node, const Split* vp);

    /**
     * @brief A debug utility that prints a node and its children.
     */
    struct PrintTree;

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

    /**
     * @brief Test whether a point is inside of a viewport.
     */
    static bool isInside(ivec2 pos, const ViewportArea& area);

    /**
     * @brief Calculate the bounding area of a tree node.
     */
    static auto getElemSize(const Node& node) -> ViewportArea;

    static void resizeElem(Node& elem, const ViewportArea& newSize);

    auto findParent(std::variant<Split*, Viewport*> elem) -> Split*;
    auto findNode(std::variant<Split*, Viewport*> elem) -> Node*;
    void mergeSplit(Split* split, ViewportLocation removedViewport);

    /**
     * @param pos A point to test for intersection with tree elements.
     *
     * Returns a split if `pos` is exactly on the boundary line between the
     * split's viewports.
     *
     * @return A viewport or a split. Nothing if `pos` is not contained by the
     *         viewport tree.
     */
    auto findElemAt(ivec2 pos) -> std::optional<std::variant<_Split, _Leaf>>;

    static constexpr i32 kViewportPadding = 3;

    Node root;
    std::vector<s_ptr<Viewport>> floatingViewports;

    // Total extent of the viewport tree.
    ViewportArea viewportArea;

    // The current cursor position in window coordinates.
    // Cached because we need it for all events to decide which viewport they
    // affect.
    ivec2 cursorPos{ 0, 0 };
};
