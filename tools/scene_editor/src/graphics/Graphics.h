#pragma once

#include <trc/core/Instance.h>
using namespace trc::basic_types;

class GraphicsStack
{
public:
    GraphicsStack();
    ~GraphicsStack() noexcept;

    auto getDevice() -> trc::Device&;
    auto getInstance() -> trc::Instance&;

private:
    trc::Instance instance;
};
