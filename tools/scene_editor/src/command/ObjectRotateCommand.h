#pragma once

#include "input/Command.h"

class ObjectRotateCommand : public Command
{
public:
    ObjectRotateCommand() = default;

    void execute(CommandExecutionContext& ctx) override;
};
