#pragma once

#include <trc/Types.h>
using namespace trc::basic_types;

#include "GraphTopology.h"

/**
 * Contains global constants about graph layouting
 */
namespace graph
{
    constexpr float kPadding{ 0.075f };

    constexpr vec2 kMinNodeSize{ 0.4f, 0.6f };

    constexpr vec2 kSocketSize{ kPadding * 2.0f };
    constexpr float kSocketSpacingHorizontal{ kSocketSize.x * 4.0f };

    constexpr float kTextHeight{ kPadding * 2.0f };
    constexpr float kNodeHeaderHeight{ kTextHeight + kPadding * 2.0f };
    constexpr vec2 kNodeContentStart{ kPadding, kPadding + kNodeHeaderHeight };

    // Padding from input field border to its contained text
    constexpr float kInputFieldInnerPadding{ kTextHeight * 0.1f };

    constexpr float kInputTextHeight{ kTextHeight * 0.8f };
    constexpr vec2 kTextInputFieldSize{ kSocketSize.x * 3.0f, kTextHeight };
} // namespace graph

struct Hitbox
{
    // The lower-left corner
    vec2 origin;

    // The extent. Upper-right corner == origin + extent
    vec2 extent;
};

/**
 * @brief Positional information about a material graph
 */
struct GraphLayout
{
    Table<Hitbox, NodeID> nodeSize;

    // Socket positions are relative to their node's position
    Table<Hitbox, SocketID> socketSize;

    // Socket decoration positions are relative to their node's position
    Table<Hitbox, SocketID> decorationSize;
};

void layoutSockets(NodeID node, const GraphTopology& graph, GraphLayout& layout);

/**
 * @brief Calculate a socket's size with all decorations
 *
 * Calculates the size of a socket and its socket decoration, if it has any.
 */
auto calcSocketSize(SocketID sock, const GraphTopology& graph) -> vec2;

/**
 * @brief Calculate a node's size
 *
 * Calculates the size of a node based on its input/output sockets
 * and additional parameters.
 *
 * @param NodeID node The node for which to calculate the size
 * @param const MaterialGraph& graph The graph in which the node resides.
 *
 * @return vec2 The node's size
 */
auto calcNodeSize(NodeID node, const GraphTopology& graph) -> vec2;

auto calcTitleTextPos(NodeID node, const GraphLayout& layout) -> vec2;

auto calcSocketGlobalPos(SocketID socket, const GraphTopology& graph, const GraphLayout& layout)
    -> vec2;

/**
 * @return bool True if `point` is inside of `hitbox`, false otherwise.
 */
bool isInside(vec2 point, const Hitbox& hitbox);
