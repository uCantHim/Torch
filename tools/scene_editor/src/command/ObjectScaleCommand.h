#pragma once

#include "input/InputCommand.h"

class ObjectScaleCommand : public InputCommand
{
public:
    ObjectScaleCommand() = default;

    void execute(CommandCall& call) override;
};
