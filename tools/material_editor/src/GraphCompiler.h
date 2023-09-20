#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <trc/material/ShaderCodePrimitives.h>
#include <trc/material/ShaderModuleBuilder.h>

#include "GraphTopology.h"

namespace code = trc::code;

struct GraphOutput
{
    std::unordered_map<std::string, code::Value> values;
};

auto compileMaterialGraph(trc::ShaderModuleBuilder& builder, const GraphTopology& graph)
    -> std::optional<GraphOutput>;
