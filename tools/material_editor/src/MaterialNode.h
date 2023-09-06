#pragma once

#include <vector>

#include <trc/material/BasicType.h>
#include <trc/material/ShaderModuleBuilder.h>

using namespace trc::basic_types;

struct SocketDescription
{
    trc::BasicType type;

    std::string name;
    std::string description;
};

struct NodeDescription
{
    // These can be inferred from the shader function signature:
    std::vector<SocketDescription> inputs;
    std::vector<SocketDescription> outputs;

    s_ptr<trc::ShaderFunction> computation;
};
