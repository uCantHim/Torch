#pragma once

#include <iostream>
#include <optional>

struct GraphScene;

void serializeGraph(const GraphScene& scene, std::ostream& os);
auto parseGraph(std::istream& is) -> std::optional<GraphScene>;
