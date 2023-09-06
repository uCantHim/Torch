#pragma once

#include <trc/Types.h>
using namespace trc::basic_types;

#include "MaterialGraph.h"

/**
 * Contains global constants about graph layouting
 */
namespace graph
{
    constexpr vec2 kMinNodeSize{ 0.4f, 0.6f };

    constexpr vec2 kSocketSize{ 0.15f, 0.15f };
    constexpr float kSocketSpacingHorizontal{ kSocketSize.x * 4 };
    constexpr float kSocketSpacingVertical{ kSocketSize.y / 2 };
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
    Table<Hitbox, SocketID> socketSize;
};

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
auto calcNodeSize(NodeID node, const MaterialGraph& graph) -> vec2;

/**
 * @return bool True if `point` is inside of `hitbox`, false otherwise.
 */
bool isInside(vec2 point, const Hitbox& hitbox);
