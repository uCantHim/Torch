#pragma once

#include "input/Command.h"

class ObjectScaleCommand : public Command
{
public:
    ObjectScaleCommand() = default;

    void execute(CommandExecutionContext& ctx) override;
};
